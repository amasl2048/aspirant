/*
 * net/sched/sch_flc.c	FLC queue
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 * Changes:
 * J Hadi Salim <hadi@nortel.com> 980914:	computation fixes
 * Alexey Makarenko <makar@phoenix.kharkov.ua> 990814: qave on idle link was calculated incorrectly.
 * J Hadi Salim <hadi@nortelnetworks.com> 980816:  ECN support	
 * A Maslennikov: FLC support
 *    - version B4 w/o log, with STAT q->st.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/notifier.h>
#include <net/ip.h>
#include <net/route.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>
#include "flc001.h"


/*	Fuzzy Logic Controller (FLC) algorithm.
	=======================================
 */

struct flc_sched_data
{
/* Parameters */
	u32		limit;		/* HARD maximal queue length	*/
	u32		qth_min;	/* Min average length threshold: A scaled */
	u32		qth_max;	/* Max average length threshold: A scaled */
	u32		Rmask;
	u32		Scell_max;
	unsigned char	flags;
	char		Wlog;		/* log(W)		*/
	char		Plog;		/* random number bits	*/
	char		Scell_log;
	u8		Stab[256];

	unsigned long  qref;
    unsigned long  d_scale;
    unsigned long  sampling;
    unsigned long  p_rate0;
    unsigned long  q_lim;
	unsigned long  r_sampl;

/* Variables */
	unsigned long	qave;		/* Average queue length: A scaled */
	int		qcount;		/* Packets since last random number generation */
	u32		qR;		/* Cached random number */

	psched_time_t	qidlestart;	/* Start of idle period		*/
	struct tc_flc_xstats st;

    int		count_bytes;
	int		count_p;
	psched_time_t		last_call_sampling;

	long		prob;
	long int	q_error;
	long int	p_rate;
	long int	d_prob;

  	struct Qdisc		*qdisc;
};

static int flc_ecn_mark(struct sk_buff *skb)
{
	if (skb->nh.raw + 20 > skb->tail)
		return 0;

	switch (skb->protocol) {
	case __constant_htons(ETH_P_IP):
		if (!INET_ECN_is_capable(skb->nh.iph->tos))
			return 0;
		if (INET_ECN_is_not_ce(skb->nh.iph->tos))
			IP_ECN_set_ce(skb->nh.iph);
		return 1;
	case __constant_htons(ETH_P_IPV6):
		if (!INET_ECN_is_capable(ip6_get_dsfield(skb->nh.ipv6h)))
			return 0;
		IP6_ECN_set_ce(skb->nh.ipv6h);
		return 1;
	default:
		return 0;
	}
}

static int
flc_enqueue(struct sk_buff *skb, struct Qdisc* sch)
{
	struct flc_sched_data *q = (struct flc_sched_data *)sch->data;

	long dt_us;	
	psched_time_t now;
    int log = 0;

	//1
	q->count_bytes += skb->len;
	++q->count_p;
    PSCHED_GET_TIME(now);
	dt_us = PSCHED_TDIFF(now, q->last_call_sampling);

    if (log){
        printk(KERN_INFO"bytes %i pakets %i \n", 
		q->count_bytes,
		q->count_p
		);

        printk(KERN_INFO"dt %li sampl %li\n", 
		dt_us,
		q->sampling*1000
		);
    }
    //2
    if (dt_us > q->sampling*1000){
      //3
	  if (log){
      printk(KERN_INFO"FLC:\n");//" backlog %i qref %i \n",
             //sch->stats.backlog, q->qref);
      }
      q->q_error = sch->stats.backlog - q->qref;
      if (q->q_error > 250000) q->q_error = 250000;
	  q->p_rate = q->count_bytes * q->r_sampl;
	  if (q->p_rate > 3750000) q->p_rate = 3750000; // 2 * 15 Mbit/s 
	  q->count_bytes = 0;
      //5
      q->d_prob = flc001(q->q_error, q->p_rate);
	  //6
      q->prob += q->d_prob;// * q->d_scale;
	  if (q->prob < 0) q->prob = 0;
	  PSCHED_GET_TIME(q->last_call_sampling);
	  
      // PRINT STAT
      q->st.q_error = q->q_error;
      q->st.p_rate = q->p_rate;
      q->st.d_prob = q->d_prob;
      q->st.prob = q->prob;

      if (log){
            printk(KERN_INFO"q %i qer %li pr %li dprob %li prob %li \n",
             sch->stats.backlog,
             q->q_error,
             q->p_rate,
             q->d_prob,
             q->prob
             ); 
      }
     }
    
	if (++q->qcount) {
      // PRINT STAT
      if (log){
       printk(KERN_INFO"prob %li qcount %i qR %u \n",
             q->prob,
             q->qcount,
             q->qR
             ); 
      }
		if (q->prob * q->qcount < q->qR)
			goto enqueue;
		q->qcount = 0;
		q->qR = net_random()&q->Rmask;
		sch->stats.overlimits++;
		goto mark;
	}
	q->qR = net_random()&q->Rmask;
	goto enqueue;

mark:
		if  (!(q->flags&TC_FLC_ECN) || !flc_ecn_mark(skb)) {
			q->st.early++;
			goto drop;
		}
		q->st.marked++;
		goto enqueue;

enqueue:
		if (sch->stats.backlog + skb->len <= q->limit) {
			__skb_queue_tail(&sch->q, skb);
			sch->stats.backlog += skb->len;
			sch->stats.bytes += skb->len;
			sch->stats.packets++;

			return NET_XMIT_SUCCESS;
		} else {
			q->st.pdrop++;
		}
		kfree_skb(skb);
		sch->stats.drops++;
     if (log){   
	// PRINT STAT
         printk(KERN_INFO" len %u limit %u \n",
               skb->len,
               q->limit
               );
     }   
		return NET_XMIT_DROP;

drop:
	kfree_skb(skb);
	sch->stats.drops++;
	return NET_XMIT_CN;
}

static int
flc_requeue(struct sk_buff *skb, struct Qdisc* sch)
{
	struct flc_sched_data *q = (struct flc_sched_data *)sch->data;

	PSCHED_SET_PASTPERFECT(q->qidlestart);

	__skb_queue_head(&sch->q, skb);
	sch->stats.backlog += skb->len;
	return 0;
}

static struct sk_buff *
flc_dequeue(struct Qdisc* sch)
{
	struct sk_buff *skb;
	struct flc_sched_data *q = (struct flc_sched_data *)sch->data;

	skb = __skb_dequeue(&sch->q);
	if (skb) {
		sch->stats.backlog -= skb->len;
		return skb;
	}
	PSCHED_GET_TIME(q->qidlestart);
	return NULL;
}

static unsigned int flc_drop(struct Qdisc* sch)
{
	struct sk_buff *skb;
	struct flc_sched_data *q = (struct flc_sched_data *)sch->data;

	skb = __skb_dequeue_tail(&sch->q);
	if (skb) {
		unsigned int len = skb->len;
		sch->stats.backlog -= len;
		sch->stats.drops++;
		q->st.other++;
		kfree_skb(skb);
		return len;
	}
	PSCHED_GET_TIME(q->qidlestart);
	return 0;
}

static void flc_reset(struct Qdisc* sch)
{
	struct flc_sched_data *q = (struct flc_sched_data *)sch->data;

	__skb_queue_purge(&sch->q);
	sch->stats.backlog = 0;
	PSCHED_SET_PASTPERFECT(q->qidlestart);
	q->qave = 0;
	q->qcount = -1;
	
    q->count_bytes 	= 0;
    q->count_p 		= 0;
    //    q->last_call_sampling 	= 0;
    q->prob 		= 0;
	q->q_error		= 0;
	q->p_rate		= 1875000; // 15 Mbit/s -> 15 / 8bytes
	q->d_prob		= 0;
}

static int flc_change(struct Qdisc *sch, struct rtattr *opt)
{
	struct flc_sched_data *q = (struct flc_sched_data *)sch->data;
	struct rtattr *tb[TCA_FLC_STAB];
	struct tc_flc_qopt *ctl;

	if (opt == NULL ||
	    rtattr_parse(tb, TCA_FLC_STAB, RTA_DATA(opt), RTA_PAYLOAD(opt)) ||
	    tb[TCA_FLC_PARMS-1] == 0 || tb[TCA_FLC_STAB-1] == 0 ||
	    RTA_PAYLOAD(tb[TCA_FLC_PARMS-1]) < sizeof(*ctl) ||
	    RTA_PAYLOAD(tb[TCA_FLC_STAB-1]) < 256)
		return -EINVAL;

	ctl = RTA_DATA(tb[TCA_FLC_PARMS-1]);

	sch_tree_lock(sch);
	q->flags = ctl->flags;
	q->Wlog = ctl->Wlog;
	q->Plog = ctl->Plog;
	q->Rmask = ctl->Plog < 32 ? ((1<<ctl->Plog) - 1) : ~0UL;
	q->Scell_log = ctl->Scell_log;
	q->Scell_max = (255<<q->Scell_log);
	q->qth_min = ctl->qth_min<<ctl->Wlog;
	q->qth_max = ctl->qth_max<<ctl->Wlog;
	q->limit = ctl->limit;

	q->qref     = ctl->qref;
    q->d_scale	= ctl->d_scale;  
    q->sampling = ctl->sampling;
    q->p_rate0	= ctl->p_rate0;
    q->q_lim	= ctl->q_lim;
	q->r_sampl	= ctl->r_sampl;

	memcpy(q->Stab, RTA_DATA(tb[TCA_FLC_STAB-1]), 256);

	q->qcount = -1;
	if (skb_queue_len(&sch->q) == 0)
		PSCHED_SET_PASTPERFECT(q->qidlestart);
	sch_tree_unlock(sch);
	return 0;
}

static int flc_init(struct Qdisc* sch, struct rtattr *opt)
{
	int err;

	MOD_INC_USE_COUNT;

	if ((err = flc_change(sch, opt)) != 0) {
		MOD_DEC_USE_COUNT;
	}
	return err;
}

int flc_copy_xstats(struct sk_buff *skb, struct tc_flc_xstats *st)
{
        RTA_PUT(skb, TCA_XSTATS, sizeof(*st), st);
        return 0;

rtattr_failure:
        return 1;
}

static int flc_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct flc_sched_data *q = (struct flc_sched_data *)sch->data;
	unsigned char	 *b = skb->tail;
	struct rtattr *rta;
	struct tc_flc_qopt opt;

	rta = (struct rtattr*)b;
	RTA_PUT(skb, TCA_OPTIONS, 0, NULL);
	opt.limit = q->limit;
	opt.qth_min = q->qth_min>>q->Wlog;
	opt.qth_max = q->qth_max>>q->Wlog;
	opt.Wlog = q->Wlog;
	opt.Plog = q->Plog;
	opt.Scell_log = q->Scell_log;
	opt.flags = q->flags;

	opt.qref     = q->qref;
    opt.d_scale	 = q->d_scale;  
    opt.sampling = q->sampling;
    opt.p_rate0	 = q->p_rate0;
    opt.q_lim	 = q->q_lim;
	opt.r_sampl	 = q->r_sampl;

	RTA_PUT(skb, TCA_FLC_PARMS, sizeof(opt), &opt);
	rta->rta_len = skb->tail - b;

	if (flc_copy_xstats(skb, &q->st))
		goto rtattr_failure;

	return skb->len;

rtattr_failure:
	skb_trim(skb, b - skb->data);
	return -1;
}

static void flc_destroy(struct Qdisc *sch)
{
	MOD_DEC_USE_COUNT;
}

struct Qdisc_ops flc_qdisc_ops =
{
	NULL,
	NULL,
	"flc",
	sizeof(struct flc_sched_data),

	flc_enqueue,
	flc_dequeue,
	flc_requeue,
	flc_drop,

	flc_init,
	flc_reset,
	flc_destroy,
	flc_change,

	flc_dump,
};


#ifdef MODULE
int init_module(void)
{
	return register_qdisc(&flc_qdisc_ops);
}

void cleanup_module(void) 
{
	unregister_qdisc(&flc_qdisc_ops);
}
#endif
MODULE_LICENSE("GPL");
