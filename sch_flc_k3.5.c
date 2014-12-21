/*
 * net/sched/sch_flc.c	
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 * Changes:
 * J Hadi Salim 980914:	computation fixes
 * Alexey Makarenko <makar@phoenix.kharkov.ua> 990814: qave on idle link was calculated incorrectly.
 * J Hadi Salim 980816:  ECN support
 * A Maslennikov: FLC support
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>
#include <net/flc.h>
#include "flc001.h"

struct flc_sched_data {
	u32			limit;		/* HARD maximal queue length */
	unsigned char		flags;
	struct timer_list	adapt_timer;
	struct flc_parms	parms;
	struct flc_vars		vars;
	struct flc_stats	stats;
	struct Qdisc		*qdisc;
};

static inline int flc_use_ecn(struct flc_sched_data *q)
{
	return q->flags & TC_FLC_ECN;
}

static inline int flc_use_harddrop(struct flc_sched_data *q)
{
	return q->flags & TC_FLC_HARDDROP;
}

static int flc_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	struct flc_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = q->qdisc;
	int ret;
	ktime_t dt;
	long dt_us;	

	//1
	q->vars.count_bytes += 1000;
	++q->vars.count_p;
	dt = ktime_sub(ktime_get(), q->vars.last_call_sampling);
        dt_us = ktime_to_us(dt);
	//2
	if (dt_us > q->parms.sampling*1000){
/*		printk(KERN_INFO"FLC: bl %i qref %i \n",
			child->qstats.backlog, q->parms.qref);
*/		//3
		q->vars.q_error = child->qstats.backlog - q->parms.qref;
		if (q->vars.q_error > 250000) q->vars.q_error = 250000;
		q->vars.p_rate = q->vars.count_bytes * q->parms.r_sampl;
		if (q->vars.p_rate > 12500000) q->vars.p_rate = 12500000; 
		q->vars.count_bytes = 0;
		//5
		q->vars.d_prob = flc001(q->vars.q_error, q->vars.p_rate);
		//6
		q->vars.prob += q->vars.d_prob;// * q->parms.d_scale;
		if (q->vars.prob < 0) q->vars.prob = 0;
		q->vars.last_call_sampling = ktime_get();
		printk(KERN_INFO"q %i qer %li pr %li dprob %li prob %li \n",
                child->qstats.backlog,
		q->vars.q_error,
                q->vars.p_rate,
                q->vars.d_prob,
                q->vars.prob
                ); 
	}

	q->vars.qavg = flc_calc_qavg(&q->parms,
				     &q->vars,
				     child->qstats.backlog);

	if (flc_is_idling(&q->vars))
		flc_end_of_idle_period(&q->vars);

	switch (flc2_action(&q->parms, &q->vars, q->vars.qavg)) {
	case FLC_DONT_MARK:
		break;

	case FLC_PROB_MARK:
		sch->qstats.overlimits++;
		if (!flc_use_ecn(q) || !INET_ECN_set_ce(skb)) {
			q->stats.prob_drop++;
			goto congestion_drop;
		}

		q->stats.prob_mark++;
		break;

	case FLC_HARD_MARK:
		sch->qstats.overlimits++;
		if (flc_use_harddrop(q) || !flc_use_ecn(q) ||
		    !INET_ECN_set_ce(skb)) {
			q->stats.forced_drop++;
			goto congestion_drop;
		}

		q->stats.forced_mark++;
		break;
	}

	ret = qdisc_enqueue(skb, child);
	if (likely(ret == NET_XMIT_SUCCESS)) {
		sch->q.qlen++;
	} else if (net_xmit_drop_count(ret)) {
		q->stats.pdrop++;
		sch->qstats.drops++;
	}
	return ret;

congestion_drop:
	qdisc_drop(skb, sch);
	return NET_XMIT_CN;
}

static struct sk_buff *flc_dequeue(struct Qdisc *sch)
{
	struct sk_buff *skb;
	struct flc_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = q->qdisc;

	skb = child->dequeue(child);
	if (skb) {
		qdisc_bstats_update(sch, skb);
		sch->q.qlen--;
	} else {
		if (!flc_is_idling(&q->vars))
			flc_start_of_idle_period(&q->vars);
	}
	return skb;
}

static struct sk_buff *flc_peek(struct Qdisc *sch)
{
	struct flc_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = q->qdisc;

	return child->ops->peek(child);
}

static unsigned int flc_drop(struct Qdisc *sch)
{
	struct flc_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = q->qdisc;
	unsigned int len;

	if (child->ops->drop && (len = child->ops->drop(child)) > 0) {
		q->stats.other++;
		sch->qstats.drops++;
		sch->q.qlen--;
		return len;
	}

	if (!flc_is_idling(&q->vars))
		flc_start_of_idle_period(&q->vars);

	return 0;
}

static void flc_reset(struct Qdisc *sch)
{
	struct flc_sched_data *q = qdisc_priv(sch);

	qdisc_reset(q->qdisc);
	sch->q.qlen = 0;
	flc_restart(&q->vars);
}

static void flc_destroy(struct Qdisc *sch)
{
	struct flc_sched_data *q = qdisc_priv(sch);

	del_timer_sync(&q->adapt_timer);
	qdisc_destroy(q->qdisc);
}

static const struct nla_policy flc_policy[TCA_FLC_MAX + 1] = {
	[TCA_FLC_PARMS]	= { .len = sizeof(struct tc_flc_qopt) },
	[TCA_FLC_STAB]	= { .len = FLC_STAB_SIZE },
	[TCA_FLC_MAX_P] = { .type = NLA_U32 },
};

static int flc_change(struct Qdisc *sch, struct nlattr *opt)
{
	struct flc_sched_data *q = qdisc_priv(sch);
	struct nlattr *tb[TCA_FLC_MAX + 1];
	struct tc_flc_qopt *ctl;
	struct Qdisc *child = NULL;
	int err;
	u32 max_P;

	if (opt == NULL)
		return -EINVAL;

	err = nla_parse_nested(tb, TCA_FLC_MAX, opt, flc_policy);
	if (err < 0)
		return err;

	if (tb[TCA_FLC_PARMS] == NULL ||
	    tb[TCA_FLC_STAB] == NULL)
		return -EINVAL;

	max_P = tb[TCA_FLC_MAX_P] ? nla_get_u32(tb[TCA_FLC_MAX_P]) : 0;

	ctl = nla_data(tb[TCA_FLC_PARMS]);

	if (ctl->limit > 0) {
		child = fifo_create_dflt(sch, &bfifo_qdisc_ops, ctl->limit);
		if (IS_ERR(child))
			return PTR_ERR(child);
	}

	sch_tree_lock(sch);
	q->flags = ctl->flags;
	q->limit = ctl->limit;
	if (child) {
		qdisc_tree_decrease_qlen(q->qdisc, q->qdisc->q.qlen);
		qdisc_destroy(q->qdisc);
		q->qdisc = child;
	}

	flc_set_parms(&q->parms,
		      ctl->qth_min, ctl->qth_max, ctl->Wlog,
		      ctl->Plog, ctl->Scell_log,
		      nla_data(tb[TCA_FLC_STAB]),
		      max_P,
			ctl->qref, ctl->d_scale, ctl->sampling,
			ctl->p_rate0, ctl->q_lim, ctl->r_sampl);
	flc_set_vars(&q->vars);

	del_timer(&q->adapt_timer);
	if (ctl->flags & TC_FLC_ADAPTATIVE)
		mod_timer(&q->adapt_timer, jiffies + HZ/2);

	if (!q->qdisc->q.qlen)
		flc_start_of_idle_period(&q->vars);

	sch_tree_unlock(sch);
	return 0;
}

static inline void flc_adaptative_timer(unsigned long arg)
{
	struct Qdisc *sch = (struct Qdisc *)arg;
	struct flc_sched_data *q = qdisc_priv(sch);
	spinlock_t *root_lock = qdisc_lock(qdisc_root_sleeping(sch));

	spin_lock(root_lock);
	flc_adaptative_algo(&q->parms, &q->vars);
	mod_timer(&q->adapt_timer, jiffies + HZ/2);
	spin_unlock(root_lock);
}

static int flc_init(struct Qdisc *sch, struct nlattr *opt)
{
	struct flc_sched_data *q = qdisc_priv(sch);

	q->qdisc = &noop_qdisc;
	setup_timer(&q->adapt_timer, flc_adaptative_timer, (unsigned long)sch);
	return flc_change(sch, opt);
}

static int flc_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct flc_sched_data *q = qdisc_priv(sch);
	struct nlattr *opts = NULL;
	struct tc_flc_qopt opt = {
		.limit		= q->limit,
		.flags		= q->flags,
		.qth_min	= q->parms.qth_min >> q->parms.Wlog,
		.qth_max	= q->parms.qth_max >> q->parms.Wlog,
		.Wlog		= q->parms.Wlog,
		.Plog		= q->parms.Plog,
		.Scell_log	= q->parms.Scell_log,

	        .qref		= q->parms.qref,
        	.d_scale	= q->parms.d_scale,  
           	.sampling	= q->parms.sampling,
                .p_rate0	= q->parms.p_rate0, 
                .q_lim		= q->parms.q_lim,
		.r_sampl	= q->parms.r_sampl,
	};

	sch->qstats.backlog = q->qdisc->qstats.backlog;
	opts = nla_nest_start(skb, TCA_OPTIONS);
	if (opts == NULL)
		goto nla_put_failure;
	if (nla_put(skb, TCA_FLC_PARMS, sizeof(opt), &opt) ||
	    nla_put_u32(skb, TCA_FLC_MAX_P, q->parms.max_P))
		goto nla_put_failure;
	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -EMSGSIZE;
}

static int flc_dump_stats(struct Qdisc *sch, struct gnet_dump *d)
{
	struct flc_sched_data *q = qdisc_priv(sch);
	struct tc_flc_xstats st = {
		.early	= q->stats.prob_drop + q->stats.forced_drop,
		.pdrop	= q->stats.pdrop,
		.other	= q->stats.other,
		.marked	= q->stats.prob_mark + q->stats.forced_mark,

		.q_error= q->vars.q_error,
		.p_rate = q->vars.p_rate,
		.d_prob = q->vars.d_prob,
		.prob	= q->vars.prob,
	};
/*	printk(KERN_INFO"STAT: qer %li pr %li dprob %li prob %li \n",
                q->vars.q_error,
                q->vars.p_rate,
                q->vars.d_prob,
                q->vars.prob
                ); 
*/	return gnet_stats_copy_app(d, &st, sizeof(st));
}

static int flc_dump_class(struct Qdisc *sch, unsigned long cl,
			  struct sk_buff *skb, struct tcmsg *tcm)
{
	struct flc_sched_data *q = qdisc_priv(sch);

	tcm->tcm_handle |= TC_H_MIN(1);
	tcm->tcm_info = q->qdisc->handle;
	return 0;
}

static int flc_graft(struct Qdisc *sch, unsigned long arg, struct Qdisc *new,
		     struct Qdisc **old)
{
	struct flc_sched_data *q = qdisc_priv(sch);

	if (new == NULL)
		new = &noop_qdisc;

	sch_tree_lock(sch);
	*old = q->qdisc;
	q->qdisc = new;
	qdisc_tree_decrease_qlen(*old, (*old)->q.qlen);
	qdisc_reset(*old);
	sch_tree_unlock(sch);
	return 0;
}

static struct Qdisc *flc_leaf(struct Qdisc *sch, unsigned long arg)
{
	struct flc_sched_data *q = qdisc_priv(sch);
	return q->qdisc;
}

static unsigned long flc_get(struct Qdisc *sch, u32 classid)
{
	return 1;
}

static void flc_put(struct Qdisc *sch, unsigned long arg)
{
}

static void flc_walk(struct Qdisc *sch, struct qdisc_walker *walker)
{
	if (!walker->stop) {
		if (walker->count >= walker->skip)
			if (walker->fn(sch, 1, walker) < 0) {
				walker->stop = 1;
				return;
			}
		walker->count++;
	}
}

static const struct Qdisc_class_ops flc_class_ops = {
	.graft		=	flc_graft,
	.leaf		=	flc_leaf,
	.get		=	flc_get,
	.put		=	flc_put,
	.walk		=	flc_walk,
	.dump		=	flc_dump_class,
};

static struct Qdisc_ops flc_qdisc_ops __read_mostly = {
	.id		=	"flc",
	.priv_size	=	sizeof(struct flc_sched_data),
	.cl_ops		=	&flc_class_ops,
	.enqueue	=	flc_enqueue,
	.dequeue	=	flc_dequeue,
	.peek		=	flc_peek,
	.drop		=	flc_drop,
	.init		=	flc_init,
	.reset		=	flc_reset,
	.destroy	=	flc_destroy,
	.change		=	flc_change,
	.dump		=	flc_dump,
	.dump_stats	=	flc_dump_stats,
	.owner		=	THIS_MODULE,
};

static int __init flc_module_init(void)
{
	return register_qdisc(&flc_qdisc_ops);
}

static void __exit flc_module_exit(void)
{
	unregister_qdisc(&flc_qdisc_ops);
}

module_init(flc_module_init)
module_exit(flc_module_exit)

MODULE_LICENSE("GPL");
