/*
 * FUZZY-Queue for TCP/IP Best-Effort networks
 * by Chrysostomos Chrysostomou
 *
 * Modifyed by A.Maslennikov 03/2011
 */

#include "flc2.h"
#include "random.h"
#include "config.h"
#include "template.h"
#include "delay.h"

// Different types of a packet drop.
#define	DTYPE_NONE	1	/* ok, no drop */
#define	DTYPE_FORCED	2	/* a "forced" drop */
#define	DTYPE_UNFORCED	3	/* an "unforced" (random) drop */
#define	DTYPE_MARKED	4	/* a mark */

extern "C" void flc_003InferenceEngine(double _d_qerror, double _d_rate, double *_d_dprob);

static class FLC2Class : public TclClass {
 public:
	FLC2Class() : TclClass("Queue/FLC2") {}
	TclObject* create(int, const char*const*) {
		return (new FLC2Queue);
	}
} class_FLC2;

/* Fuzzy Queue */

void FLC2Queue::reset()
{
  old_currsize = 0;
  currsize = 0;
  avg_total =  0;
  queue_error = queue_error_old = 0;
  prob_new = 0.0;
  count_p = 0;
  count_bytes = 0;
  dt = 0.0;
  last_call_sampling = 0.0;
  prob = 0;
  qerror_norm_tr = qerror_old_norm_tr = prate_tr = 0.0;
  p_rate = 0;	// current packet arriaval rate per sec
	
  empty = 1;
  if (&Scheduler::instance() != NULL)
    {
      emptytime = Scheduler::instance().clock();
    }
  else
    {
      emptytime = 0.0;    /* sched not instantiated yet */
    }

    Queue::reset();
}

/*
 * FUZZY queue
 */
void FLC2Queue::enque(Packet* p)
{
 //normalized values of the two input variables of the inference engines
	double qerror_norm; //qerror_old_norm;

  // This field expresses the type of drop
  int droptype = DTYPE_NONE;

  // access packet's headers
hdr_cmn* ch = hdr_cmn::access(p);
// Number of bytes considered to be enqueued
count_bytes = count_bytes + ch->size();
// Number of packets considered to be enqueued
++count_p;

  hdr_flags* hf = hdr_flags::access(p);

  //get current (simulation) time
  double now = Scheduler::instance().clock();

// Time ellapsed since the last sampling of queue
dt = now-last_call_sampling;

// Sapling period for finding the errors on queue
 if (dt > sampling_period_queue)
   {
     // Calculating the error on queue for two consecutive sample intervals
     queue_error = fuzzyQueue_->length() - queue_des ;
     // current bytes arriaval rate per sec
     p_rate = count_bytes/sampling_period_queue;
     
     count_bytes = 0;

     // Normalize the values of errors on queue:
     // use the input scaling gains: [1/(q_des-qlim) , 1/q_des]
	
     if (queue_error >= 0)
       qerror_norm = queue_error/(qlim_ - queue_des);
	
     if (queue_error < 0)
       {
	 if (queue_des == 0)
	   qerror_norm = queue_error;
	 else
	   qerror_norm = queue_error/queue_des;
       }
	
     // Normalize the values of d_prate:
     prate_norm = p_rate/p_rate0 - 1;
     if (prate_norm > 1) prate_norm = 1;

     // Trace the normalized values of errors on queue
     qerror_norm_tr = qerror_norm;
     prate_tr = prate_norm;

     // Use the Fuzzy Inference Engine:
     // normalised inputs: error of queue length ans rate
     // output: the drop/mark probability

       flc_003InferenceEngine(qerror_norm, prate_norm, &d_prob);

     prob = prob + d_prob*d_scale;
     if (prob < 0) prob = 0;

     // The probability to be used to drop/mark the packet
     //	prob_new = prob*prob_max; 
     prob_new = prob;

     // Trace the drop/mark probability
     d_prob_tr = d_prob;	// FLC output before scale
     prob_tr = prob_new;	// P drop after scale and additive

     last_call_sampling = now;

   } // end of sampling period


  /* if queue is already full, drop packet */
  if (fuzzyQueue_->length() >= qlim_)
  {
      drop(p);
      droptype = DTYPE_FORCED;
      count_p = 0;
  }
  else // use the prob taken from FIE to decide whether to drop/mark the packet
  {
  	if ( fuzzyQueue_->length() > 1 )
	{

	// In order to drop/mark a packet, we must have a minimum of packets to be enqueued
	// e.g. for a prob of 0.01 (1 out of 100), we need at least 100 packets to be enqueued to drop/mark the packet
		if (count_p * prob_new < 1.0)
			prob_new = 0.0;
		else if (count_p * prob_new < 2.0)
			prob_new /= (2 - count_p * prob_new);
		else
			prob_new = 1.0;
		
	// check values of prob ranging from 0 to 1.
       if (prob_new < 0)
       		prob_new = 0.0;
       if (prob_new > 1.0)
       		prob_new = 1.0;

		double u =Random::uniform();

       		if (u <= prob_new)
		{
			count_p = 0; // initialize count_p to 0 when a packet is to be marked/dropped

			// if ECN enabled, mark the packet
      			if (setbit && hf->ect())
         		{
				hf->ce() = 1; 	// mark Congestion Experienced bit
				droptype = DTYPE_MARKED;
			}
			else // if ECN is not enabled, drop the packet
			{
				drop(p);
				droptype = DTYPE_UNFORCED;
			}
		}
	}

	if (droptype != DTYPE_UNFORCED)
	{
		fuzzyQueue_->enque(p);
		currsize = fuzzyQueue_->length(); // to trace the current queue length
		empty = 0;
	}
  } // end of use of FIE output for marking/dropping the packet

} // end of enqueue function

Packet* FLC2Queue::deque()
{
  Packet *p;
  hdr_cmn* hdr;

    p = fuzzyQueue_->deque();
    if (p != 0) {
//      hdr_ip* iph = hdr_ip::access(p); // not used
      hdr = hdr_cmn::access(p);
      }

    if (fuzzyQueue_->length() == 0) {
      /* if FUZZY queue becomes empty, record time */
      if (empty == 0) {
				/* queue was not empty before */
	empty = 1;
	if (&Scheduler::instance() != NULL) {
	  emptytime = Scheduler::instance().clock();
	} else {
	  emptytime = 0.0;
	}
      }
    }
  return (p);
}

int FLC2Queue::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 3) {
		// attach a file for variable tracing
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("FLC2: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}

		// tell FLC2 about link stats
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("FLC2: no LinkDelay object %s",
					argv[2]);
				return(TCL_ERROR);
			}
			// set ptc now
			link_ = del;
			ptc = link_->bandwidth() / (8.0 * mean_pktsize);

			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}

void FLC2Queue::trace(TracedVar* v)
{
	char wrk[500], *p;

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		// XXX: be compatible with nsv1 RED trace entries
		if (strstr(v->name(), "curq_") != NULL) {
			sprintf(wrk, "Q %g %d", t, int(*((TracedInt*) v)));
		}
		if (strstr(v->name(), "ave_") != NULL) {
			sprintf(wrk, "A %g %g", t, double(*((TracedDouble*) v)));
		}
		if (strstr(v->name(), "qerror_norm_") != NULL) {
			sprintf(wrk, "Qerr\t %g\t %g", t, double(*((TracedDouble*) v)));
		}
		if (strstr(v->name(), "qerror_old_norm_") != NULL) {
			sprintf(wrk, "Qerror_old_norm %g %g", t, double(*((TracedDouble*) v)));
		}
		if (strstr(v->name(), "p_rate_") != NULL) {
			sprintf(wrk, "Prate\t %g\t %g", t, double(*((TracedDouble*) v)));
		}
		if (strstr(v->name(), "prob_") != NULL) {
			sprintf(wrk, "Prob\t %g\t %g", t, double(*((TracedDouble*) v)));
		}
		if (strstr(v->name(), "d_prob_") != NULL) {
			sprintf(wrk, "d_Pr\t %g\t %g", t, double(*((TracedDouble*) v)));
		}
		if (strstr(v->name(), "kf_") != NULL) {
			sprintf(wrk, "kf\t %g\t %g", t, double(*((TracedDouble*) v)));
		}
		n = strlen(wrk);
		wrk[n] = '\n';
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
	}
	return;
}

