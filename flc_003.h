#ifndef XFUZZY_INFERENCE_ENGINE
#define XFUZZY_INFERENCE_ENGINE

typedef struct {
 double (* and)();
 double (* or)();
 double (* also)();
 double (* imp)();
 double (* not)();
 double (* very)();
 double (* moreorless)();
 double (* slightly)();
 double (* defuz)();
} OperatorSet;

typedef struct {
 double min;
 double max;
 double step;
 double *param;
 double (* compute_eq)();
 double (* compute_greq)();
 double (* compute_smeq)();
 double (* center)();
 double (* basis)();
} MembershipFunction;

typedef struct {
 int crisp;
 double crispvalue;
 int length;
 double *degree;
 MembershipFunction *conc;
 int inputlength;
 double *input;
 OperatorSet op;
} FuzzyNumber;


#endif /* XFUZZY_INFERENCE_ENGINE */

#ifndef XFUZZY_flc_003
#define XFUZZY_flc_003


typedef struct {
 MembershipFunction nh;
 MembershipFunction nb;
 MembershipFunction ns;
 MembershipFunction z;
 MembershipFunction ps;
 MembershipFunction pb;
 MembershipFunction ph;
} TP_Input;

typedef struct {
 MembershipFunction nh;
 MembershipFunction nb;
 MembershipFunction ns;
 MembershipFunction nt;
 MembershipFunction z;
 MembershipFunction t;
 MembershipFunction s;
 MembershipFunction b;
 MembershipFunction h;
} TP_Output;

typedef struct {
 MembershipFunction nh;
 MembershipFunction nb;
 MembershipFunction ns;
 MembershipFunction z;
 MembershipFunction s;
 MembershipFunction b;
 MembershipFunction h;
} TP_Input2;
 double flc_003IE();

#endif /* XFUZZY_flc_003 */
