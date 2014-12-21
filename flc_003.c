#include <stdio.h>
#include <math.h>
#include "flc_003.h"

/***********************************************/
/*   Default methods for membership functions  */
/***********************************************/

static double _defaultMFcompute_smeq(MembershipFunction mf, double x) {
 double y,mu,degree=0;
 for(y=mf.max; y>=x ; y-=mf.step)
  if((mu = mf.compute_eq(mf,y))>degree) degree=mu;
 return degree;
}

static double _defaultMFcompute_greq(MembershipFunction mf, double x) {
 double y,mu,degree=0;
 for(y=mf.min; y<=x ; y+=mf.step)
   if((mu = mf.compute_eq(mf,y))>degree) degree=mu;
 return degree;
}

static double _defaultMFcenter(MembershipFunction mf) {
 return 0;
}

static double _defaultMFbasis(MembershipFunction mf) {
 return 0;
}

/***********************************************/
/* Common functions for defuzzification methods*/
/***********************************************/

static double compute(FuzzyNumber fn, double x) {
 double dom,imp;
 int i;
 if(fn.length == 0) return 0;
 imp = fn.op.imp(fn.degree[0],fn.conc[0].compute_eq(fn.conc[0],x));
 dom = imp;
 for(i=1; i<fn.length; i++) {
  imp = fn.op.imp(fn.degree[i],fn.conc[i].compute_eq(fn.conc[i],x));
  dom = fn.op.also(dom,imp);
 }
 return dom;
}

static double center(MembershipFunction mf) {
 return mf.center(mf);
}

static double basis(MembershipFunction mf) {
 return mf.basis(mf);
}

static double param(MembershipFunction mf,int i) {
 return mf.param[i];
}

/***********************************************/
/*  Common functions to create fuzzy numbers   */
/***********************************************/

static double MF_xfl_singleton_equal(MembershipFunction _mf,double x);

static FuzzyNumber createCrispNumber(double value) {
 FuzzyNumber fn;
 fn.crisp = 1;
 fn.crispvalue = value;
 fn.length = 0;
 fn.degree = NULL;
 fn.conc = NULL;
 fn.inputlength = 0;
 fn.input = NULL;
 return fn;
}

static FuzzyNumber createFuzzyNumber(int length, int inputlength,
                                     double*input, OperatorSet op) {
 int i;
 FuzzyNumber fn;
 fn.crisp = 0;
 fn.crispvalue = 0;
 fn.length = length;
 fn.degree = (double *) malloc(length*sizeof(double));
 fn.conc = (MembershipFunction *) malloc(length*sizeof(MembershipFunction));
 for(i=0; i<length; i++) fn.degree[i] = 0;
 fn.inputlength = inputlength;
 fn.input = input;
 fn.op = op;
 return fn;
}

static int isDiscreteFuzzyNumber(FuzzyNumber fn) {
 int i;
 if(fn.crisp) return 0;
 for(i=0; i<fn.length; i++)
  if(fn.conc[i].compute_eq != MF_xfl_singleton_equal)
   return 0;
 return 1;
}

/***********************************************/
/*  Functions to compute single propositions   */
/***********************************************/

static double _isEqual(MembershipFunction mf, FuzzyNumber fn) {
 int i;
 double x,mu1,mu2,minmu,degree=0;
 if(fn.crisp) return mf.compute_eq(mf,fn.crispvalue);
 if( isDiscreteFuzzyNumber(fn) ) {
  for(i=0; i<fn.length; i++){
   mu1 = mf.compute_eq(mf,fn.conc[i].param[0]);
   minmu = (mu1<fn.degree[i] ? mu1 : fn.degree[i]);
   if( degree<minmu ) degree = minmu;
  }
 }
 else {
  for(x=mf.min; x<=mf.max; x+=mf.step){
   mu1 = mf.compute_eq(mf,x);
   mu2 = compute(fn,x);
   minmu = (mu1<mu2 ? mu1 : mu2);
   if( degree<minmu ) degree = minmu;
  }
 }
 return degree;
}

static double _isGreaterOrEqual(MembershipFunction mf, FuzzyNumber fn) {
 int i;
 double x,mu1,mu2,minmu,degree=0,greq=0;
 if(fn.crisp) return mf.compute_greq(mf,fn.crispvalue);
 if( isDiscreteFuzzyNumber(fn) ) {
  for(i=0; i<fn.length; i++){
   mu1 = mf.compute_greq(mf,fn.conc[i].param[0]);
   minmu = (mu1<fn.degree[i] ? mu1 : fn.degree[i]);
   if( degree<minmu ) degree = minmu;
  }
 }
 else {
  for(x=mf.min; x<=mf.max; x+=mf.step){
   mu1 = compute(fn,x);
   mu2 = mf.compute_eq(mf,x);
   if( mu2>greq ) greq = mu2;
   if( mu1<greq ) minmu = mu1; else minmu = greq;
   if( degree<minmu ) degree = minmu;
  }
 }
 return degree;
}

static double _isSmallerOrEqual(MembershipFunction mf, FuzzyNumber fn) {
 int i;
 double x,mu1,mu2,minmu,degree=0,smeq=0;
 if(fn.crisp) return mf.compute_smeq(mf,fn.crispvalue);
 if( isDiscreteFuzzyNumber(fn) ) {
  for(i=0; i<fn.length; i++){
   mu1 = mf.compute_smeq(mf,fn.conc[i].param[0]);
   minmu = (mu1<fn.degree[i] ? mu1 : fn.degree[i]);
   if( degree<minmu ) degree = minmu;
  }
 }
 else {
  for(x=mf.max; x>=mf.min; x-=mf.step){
   mu1 = compute(fn,x);
   mu2 = mf.compute_eq(mf,x);
   if( mu2>smeq ) smeq = mu2;
   if( mu1<smeq ) minmu = mu1; else minmu = smeq;
   if( degree<minmu ) degree = minmu;
  }
 }
 return degree;
}

static double _isGreater(MembershipFunction mf, FuzzyNumber fn, OperatorSet op) {
 int i;
 double x,mu1,mu2,minmu,gr,degree=0,smeq=0;
 if(fn.crisp) return op.not(mf.compute_smeq(mf,fn.crispvalue));
 if( isDiscreteFuzzyNumber(fn) ) {
  for(i=0; i<fn.length; i++){
   mu1 = op.not(mf.compute_smeq(mf,fn.conc[i].param[0]));
   minmu = (mu1<fn.degree[i] ? mu1 : fn.degree[i]);
   if( degree<minmu ) degree = minmu;
  }
 }
 else {
  for(x=mf.max; x>=mf.min; x-=mf.step){
   mu1 = compute(fn,x);
   mu2 = mf.compute_eq(mf,x);
   if( mu2>smeq ) smeq = mu2;
   gr = op.not(smeq);
   minmu = ( mu1<gr ? mu1 : gr);
   if( degree<minmu ) degree = minmu;
  }
 }
 return degree;
}

static double _isSmaller(MembershipFunction mf, FuzzyNumber fn, OperatorSet op) {
 int i;
 double x,mu1,mu2,minmu,sm,degree=0,greq=0;
 if(fn.crisp) return op.not(mf.compute_greq(mf,fn.crispvalue));
 if( isDiscreteFuzzyNumber(fn) ) {
  for(i=0; i<fn.length; i++){
   mu1 = op.not(mf.compute_greq(mf,fn.conc[i].param[0]));
   minmu = (mu1<fn.degree[i] ? mu1 : fn.degree[i]);
   if( degree<minmu ) degree = minmu;
  }
 }
 else {
  for(x=mf.min; x<=mf.max; x+=mf.step){
   mu1 = compute(fn,x);
   mu2 = mf.compute_eq(mf,x);
   if( mu2>greq ) greq = mu2;
   sm = op.not(greq);
   minmu = ( mu1<sm ? mu1 : sm);
   if( degree<minmu ) degree = minmu;
  }
 }
 return degree;
}

static double _isNotEqual(MembershipFunction mf, FuzzyNumber fn, OperatorSet op) {
 int i;
 double x,mu1,mu2,minmu,degree=0;
 if(fn.crisp) return op.not(mf.compute_eq(mf,fn.crispvalue));
 if( isDiscreteFuzzyNumber(fn) ) {
  for(i=0; i<fn.length; i++){
   mu1 = op.not(mf.compute_eq(mf,fn.conc[i].param[0]));
   minmu = (mu1<fn.degree[i] ? mu1 : fn.degree[i]);
   if( degree<minmu ) degree = minmu;
  }
 }
 else {
  for(x=mf.min; x<=mf.max; x+=mf.step){
   mu1 = compute(fn,x);
   mu2 = op.not(mf.compute_eq(mf,x));
   minmu = (mu1<mu2 ? mu1 : mu2);
   if( degree<minmu ) degree = minmu;
  }
 }
 return degree;
}

static double _isApproxEqual(MembershipFunction mf, FuzzyNumber fn, OperatorSet op) {
 int i;
 double x,mu1,mu2,minmu,degree=0;
 if(fn.crisp) return op.moreorless(mf.compute_eq(mf,fn.crispvalue));
 if( isDiscreteFuzzyNumber(fn) ) {
  for(i=0; i<fn.length; i++){
   mu1 = op.moreorless(mf.compute_eq(mf,fn.conc[i].param[0]));
   minmu = (mu1<fn.degree[i] ? mu1 : fn.degree[i]);
   if( degree<minmu ) degree = minmu;
  }
 }
 else {
  for(x=mf.min; x<=mf.max; x+=mf.step){
   mu1 = compute(fn,x);
   mu2 = op.moreorless(mf.compute_eq(mf,x));
   minmu = (mu1<mu2 ? mu1 : mu2);
   if( degree<minmu ) degree = minmu;
  }
 }
 return degree;
}

static double _isVeryEqual(MembershipFunction mf, FuzzyNumber fn, OperatorSet op) {
 int i;
 double x,mu1,mu2,minmu,degree=0;
 if(fn.crisp) return op.very(mf.compute_eq(mf,fn.crispvalue));
 if( isDiscreteFuzzyNumber(fn) ) {
  for(i=0; i<fn.length; i++){
   mu1 = op.very(mf.compute_eq(mf,fn.conc[i].param[0]));
   minmu = (mu1<fn.degree[i] ? mu1 : fn.degree[i]);
   if( degree<minmu ) degree = minmu;
  }
 }
 else {
  for(x=mf.min; x<=mf.max; x+=mf.step){
   mu1 = compute(fn,x);
   mu2 = op.very(mf.compute_eq(mf,x));
   minmu = (mu1<mu2 ? mu1 : mu2);
   if( degree<minmu ) degree = minmu;
  }
 }
 return degree;
}

static double _isSlightlyEqual(MembershipFunction mf, FuzzyNumber fn, OperatorSet op) {
 int i;
 double x,mu1,mu2,minmu,degree=0;
 if(fn.crisp) return op.slightly(mf.compute_eq(mf,fn.crispvalue));
 if( isDiscreteFuzzyNumber(fn) ) {
  for(i=0; i<fn.length; i++){
   mu1 = op.slightly(mf.compute_eq(mf,fn.conc[i].param[0]));
   minmu = (mu1<fn.degree[i] ? mu1 : fn.degree[i]);
   if( degree<minmu ) degree = minmu;
  }
 }
 else {
  for(x=mf.min; x<=mf.max; x+=mf.step){
   mu1 = compute(fn,x);
   mu2 = op.slightly(mf.compute_eq(mf,x));
   minmu = (mu1<mu2 ? mu1 : mu2);
   if( degree<minmu ) degree = minmu;
  }
 }
 return degree;
}


/***************************************/
/*  MembershipFunction MF_xfl_trapezoid  */
/***************************************/
static double MF_xfl_trapezoid_equal(MembershipFunction _mf,double x) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
 double d = _mf.param[3];
  return (x<a || x>d? 0: (x<b? (x-a)/(b-a) : (x<c?1 : (d-x)/(d-c)))); 
}
static double MF_xfl_trapezoid_greq(MembershipFunction _mf,double x) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
 double d = _mf.param[3];
  return (x<a? 0 : (x>b? 1 : (x-a)/(b-a) )); 
}
static double MF_xfl_trapezoid_smeq(MembershipFunction _mf,double x) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
 double d = _mf.param[3];
  return (x<c? 1 : (x>d? 0 : (d-x)/(d-c) )); 
}
static double MF_xfl_trapezoid_center(MembershipFunction _mf) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
 double d = _mf.param[3];
  return (b+c)/2; 
}
static double MF_xfl_trapezoid_basis(MembershipFunction _mf) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
 double d = _mf.param[3];
  return (d-a); 
}
static MembershipFunction createMF_xfl_trapezoid( double min, double max, double step, double *param,int length) {
 int i;
 MembershipFunction _mf;
 _mf.min = min;
 _mf.max = max;
 _mf.step = step;
 _mf.param = (double*) malloc(length*sizeof(double));
 for(i=0;i<length;i++) _mf.param[i] = param[i];
 _mf.compute_eq = MF_xfl_trapezoid_equal;
 _mf.compute_greq = MF_xfl_trapezoid_greq;
 _mf.compute_smeq = MF_xfl_trapezoid_smeq;
 _mf.center = MF_xfl_trapezoid_center;
 _mf.basis = MF_xfl_trapezoid_basis;
 return _mf;
}

/***************************************/
/*  MembershipFunction MF_xfl_triangle  */
/***************************************/
static double MF_xfl_triangle_equal(MembershipFunction _mf,double x) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
  return (a<x && x<=b? (x-a)/(b-a) : (b<x && x<c? (c-x)/(c-b) : 0)); 
}
static double MF_xfl_triangle_greq(MembershipFunction _mf,double x) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
  return (x<a? 0 : (x>b? 1 : (x-a)/(b-a) )); 
}
static double MF_xfl_triangle_smeq(MembershipFunction _mf,double x) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
  return (x<b? 1 : (x>c? 0 : (c-x)/(c-b) )); 
}
static double MF_xfl_triangle_center(MembershipFunction _mf) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
  return b; 
}
static double MF_xfl_triangle_basis(MembershipFunction _mf) {
 double a = _mf.param[0];
 double b = _mf.param[1];
 double c = _mf.param[2];
  return (c-a); 
}
static MembershipFunction createMF_xfl_triangle( double min, double max, double step, double *param,int length) {
 int i;
 MembershipFunction _mf;
 _mf.min = min;
 _mf.max = max;
 _mf.step = step;
 _mf.param = (double*) malloc(length*sizeof(double));
 for(i=0;i<length;i++) _mf.param[i] = param[i];
 _mf.compute_eq = MF_xfl_triangle_equal;
 _mf.compute_greq = MF_xfl_triangle_greq;
 _mf.compute_smeq = MF_xfl_triangle_smeq;
 _mf.center = MF_xfl_triangle_center;
 _mf.basis = MF_xfl_triangle_basis;
 return _mf;
}

/***************************************/
/*  MembershipFunction MF_xfl_singleton  */
/***************************************/
static double MF_xfl_singleton_equal(MembershipFunction _mf,double x) {
 double a = _mf.param[0];
  return (x==a? 1 : 0); 
}
static double MF_xfl_singleton_greq(MembershipFunction _mf,double x) {
 double a = _mf.param[0];
  return (x>=a? 1 : 0); 
}
static double MF_xfl_singleton_smeq(MembershipFunction _mf,double x) {
 double a = _mf.param[0];
  return (x<=a? 1 : 0); 
}
static double MF_xfl_singleton_center(MembershipFunction _mf) {
 double a = _mf.param[0];
  return a; 
}
static MembershipFunction createMF_xfl_singleton( double min, double max, double step, double *param,int length) {
 int i;
 MembershipFunction _mf;
 _mf.min = min;
 _mf.max = max;
 _mf.step = step;
 _mf.param = (double*) malloc(length*sizeof(double));
 for(i=0;i<length;i++) _mf.param[i] = param[i];
 _mf.compute_eq = MF_xfl_singleton_equal;
 _mf.compute_greq = MF_xfl_singleton_greq;
 _mf.compute_smeq = MF_xfl_singleton_smeq;
 _mf.center = MF_xfl_singleton_center;
 _mf.basis = _defaultMFbasis;
 return _mf;
}
/***************************************/
/*  Operatorset OP_my_set */
/***************************************/

static double OP_my_set_And(double a, double b) {
  return (a<b? a : b); 
}

static double OP_my_set_Or(double a, double b) {
  return (a>b? a : b); 
}

static double OP_my_set_Also(double a, double b) {
  return (a>b? a : b); 
}

static double OP_my_set_Imp(double a, double b) {
  return (a<b? a : b); 
}

static double OP_my_set_Not(double a) {
  return 1-a; 
}

static double OP_my_set_Very(double a) {
 double w = 2.0;
  return pow(a,w); 
}

static double OP_my_set_MoreOrLess(double a) {
 double w = 0.5;
  return pow(a,w); 
}

static double OP_my_set_Slightly(double a) {
  return 4*a*(1-a); 
}

static double OP_my_set_Defuz(FuzzyNumber mf) {
 double min = mf.conc[0].min;
 double max = mf.conc[0].max;
 double step = mf.conc[0].step;
   double x, m, num=0, denom=0;
   for(x=min; x<=max; x+=step) {
    m = compute(mf,x);
    num += x*m;
    denom += m;
   }
   if(denom==0) return (min+max)/2;
   return num/denom;
}

static OperatorSet createOP_my_set() {
 OperatorSet op;
 op.and = OP_my_set_And;
 op.or = OP_my_set_Or;
 op.also = OP_my_set_Also;
 op.imp = OP_my_set_Imp;
 op.not = OP_my_set_Not;
 op.very = OP_my_set_Very;
 op.moreorless = OP_my_set_MoreOrLess;
 op.slightly = OP_my_set_Slightly;
 op.defuz = OP_my_set_Defuz;
 return op;
}


/***************************************/
/*  Type TP_Input */
/***************************************/

static TP_Input createTP_Input() {
 TP_Input tp;
 double min = -1.0;
 double max = 1.0;
 double step = 0.003913894324853229;
 double _p_nh[4] = { -1.5,-1.0,-0.75,-0.5 };
 double _p_nb[3] = { -0.75,-0.5,-0.25 };
 double _p_ns[3] = { -0.5,-0.25,-0.05 };
 double _p_z[4] = { -0.25,-0.05,0.05,0.25 };
 double _p_ps[3] = { 0.05,0.25,0.5 };
 double _p_pb[3] = { 0.25,0.5,0.75 };
 double _p_ph[4] = { 0.5,0.75,1.0,1.5 };
 tp.nh = createMF_xfl_trapezoid(min,max,step,_p_nh,4);
 tp.nb = createMF_xfl_triangle(min,max,step,_p_nb,3);
 tp.ns = createMF_xfl_triangle(min,max,step,_p_ns,3);
 tp.z = createMF_xfl_trapezoid(min,max,step,_p_z,4);
 tp.ps = createMF_xfl_triangle(min,max,step,_p_ps,3);
 tp.pb = createMF_xfl_triangle(min,max,step,_p_pb,3);
 tp.ph = createMF_xfl_trapezoid(min,max,step,_p_ph,4);
 return tp;
}

/***************************************/
/*  Type TP_Output */
/***************************************/

static TP_Output createTP_Output() {
 TP_Output tp;
 double min = -1.3;
 double max = 1.3;
 double step = 0.005088062622309198;
 double _p_nh[4] = { -1.5,-1.3,-0.8,-0.6 };
 double _p_nb[3] = { -0.8,-0.6,-0.3999999999999999 };
 double _p_ns[3] = { -0.6,-0.3999999999999999,-0.19999999999999996 };
 double _p_nt[3] = { -0.3999999999999999,-0.19999999999999996,0.0 };
 double _p_z[3] = { -0.19999999999999996,0.0,0.20000000000000018 };
 double _p_t[3] = { 0.0,0.20000000000000018,0.40000000000000013 };
 double _p_s[3] = { 0.20000000000000018,0.40000000000000013,0.6000000000000001 };
 double _p_b[3] = { 0.40000000000000013,0.6000000000000001,0.8 };
 double _p_h[4] = { 0.6,0.8,1.3,1.5 };
 tp.nh = createMF_xfl_trapezoid(min,max,step,_p_nh,4);
 tp.nb = createMF_xfl_triangle(min,max,step,_p_nb,3);
 tp.ns = createMF_xfl_triangle(min,max,step,_p_ns,3);
 tp.nt = createMF_xfl_triangle(min,max,step,_p_nt,3);
 tp.z = createMF_xfl_triangle(min,max,step,_p_z,3);
 tp.t = createMF_xfl_triangle(min,max,step,_p_t,3);
 tp.s = createMF_xfl_triangle(min,max,step,_p_s,3);
 tp.b = createMF_xfl_triangle(min,max,step,_p_b,3);
 tp.h = createMF_xfl_trapezoid(min,max,step,_p_h,4);
 return tp;
}

/***************************************/
/*  Type TP_Input2 */
/***************************************/

static TP_Input2 createTP_Input2() {
 TP_Input2 tp;
 double min = -1.0;
 double max = 1.0;
 double step = 0.003913894324853229;
 double _p_nh[4] = { -1.5,-1.0,-0.75,-0.5 };
 double _p_nb[3] = { -0.75,-0.5,-0.25 };
 double _p_ns[3] = { -0.5,-0.25,-0.05 };
 double _p_z[4] = { -0.25,-0.05,0.05,0.25 };
 double _p_s[3] = { 0.05,0.25,0.5 };
 double _p_b[3] = { 0.25,0.5,0.75 };
 double _p_h[4] = { 0.5,0.75,1.0,1.5 };
 tp.nh = createMF_xfl_trapezoid(min,max,step,_p_nh,4);
 tp.nb = createMF_xfl_triangle(min,max,step,_p_nb,3);
 tp.ns = createMF_xfl_triangle(min,max,step,_p_ns,3);
 tp.z = createMF_xfl_trapezoid(min,max,step,_p_z,4);
 tp.s = createMF_xfl_triangle(min,max,step,_p_s,3);
 tp.b = createMF_xfl_triangle(min,max,step,_p_b,3);
 tp.h = createMF_xfl_trapezoid(min,max,step,_p_h,4);
 return tp;
}

/***************************************/
/*  Rulebase RL_base */
/***************************************/

static void RL_base(FuzzyNumber qerror, FuzzyNumber rate, FuzzyNumber *dprob) {
 OperatorSet _op = createOP_my_set();
 double _rl, _output;
 int _i_dprob=0;
 TP_Input _t_qerror = createTP_Input();
 TP_Input2 _t_rate = createTP_Input2();
 TP_Output _t_dprob = createTP_Output();
 double *_input = (double*) malloc(2*sizeof(double));
 _input[0] = qerror.crispvalue;
 _input[1] = rate.crispvalue;
 *dprob = createFuzzyNumber(49,2,_input,_op);
 _rl = _op.and(_isEqual(_t_qerror.nh,qerror),_isEqual(_t_rate.nh,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nh;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nh,qerror),_isEqual(_t_rate.nb,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nh;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nh,qerror),_isEqual(_t_rate.ns,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nh;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nh,qerror),_isEqual(_t_rate.z,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nh;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nh,qerror),_isEqual(_t_rate.s,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nb;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nh,qerror),_isEqual(_t_rate.b,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nb;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nh,qerror),_isEqual(_t_rate.h,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nb;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nb,qerror),_isEqual(_t_rate.nh,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nh;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nb,qerror),_isEqual(_t_rate.nb,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nh;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nb,qerror),_isEqual(_t_rate.ns,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nb;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nb,qerror),_isEqual(_t_rate.z,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nb;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nb,qerror),_isEqual(_t_rate.s,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.ns;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nb,qerror),_isEqual(_t_rate.b,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.ns;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.nb,qerror),_isEqual(_t_rate.h,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.ns;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ns,qerror),_isEqual(_t_rate.nh,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nh;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ns,qerror),_isEqual(_t_rate.nb,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nb;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ns,qerror),_isEqual(_t_rate.ns,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.ns;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ns,qerror),_isEqual(_t_rate.z,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.ns;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ns,qerror),_isEqual(_t_rate.s,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nt;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ns,qerror),_isEqual(_t_rate.b,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nt;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ns,qerror),_isEqual(_t_rate.h,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nt;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.z,qerror),_isEqual(_t_rate.nh,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nb;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.z,qerror),_isEqual(_t_rate.nb,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nb;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.z,qerror),_isEqual(_t_rate.ns,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nt;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.z,qerror),_isEqual(_t_rate.z,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.z;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.z,qerror),_isEqual(_t_rate.s,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.z;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.z,qerror),_isEqual(_t_rate.b,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.t;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.z,qerror),_isEqual(_t_rate.h,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.t;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ps,qerror),_isEqual(_t_rate.nh,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.ns;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ps,qerror),_isEqual(_t_rate.nb,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.ns;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ps,qerror),_isEqual(_t_rate.ns,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nt;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ps,qerror),_isEqual(_t_rate.z,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.t;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ps,qerror),_isEqual(_t_rate.s,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.t;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ps,qerror),_isEqual(_t_rate.b,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.s;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ps,qerror),_isEqual(_t_rate.h,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.s;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.pb,qerror),_isEqual(_t_rate.nh,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nt;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.pb,qerror),_isEqual(_t_rate.nb,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.nt;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.pb,qerror),_isEqual(_t_rate.ns,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.z;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.pb,qerror),_isEqual(_t_rate.z,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.s;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.pb,qerror),_isEqual(_t_rate.s,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.s;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.pb,qerror),_isEqual(_t_rate.b,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.b;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.pb,qerror),_isEqual(_t_rate.h,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.b;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ph,qerror),_isEqual(_t_rate.nh,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.z;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ph,qerror),_isEqual(_t_rate.nb,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.z;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ph,qerror),_isEqual(_t_rate.ns,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.t;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ph,qerror),_isEqual(_t_rate.z,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.b;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ph,qerror),_isEqual(_t_rate.s,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.b;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ph,qerror),_isEqual(_t_rate.b,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.h;
 _i_dprob++;
 _rl = _op.and(_isEqual(_t_qerror.ph,qerror),_isEqual(_t_rate.h,rate));
 dprob->degree[_i_dprob] = _rl;
 dprob->conc[_i_dprob] =  _t_dprob.h;
 _i_dprob++;
 _output = _op.defuz(*dprob);
 free(dprob->degree);
 free(dprob->conc);
 *dprob = createCrispNumber(_output);
}

/***************************************/
/*          Inference Engine           */
/***************************************/

double flc_003IE(double _d_qerror, double _d_rate) {
 double _d_dprob;
 FuzzyNumber qerror = createCrispNumber(_d_qerror);
 FuzzyNumber rate = createCrispNumber(_d_rate);
 FuzzyNumber dprob;
 RL_base(qerror, rate, &dprob);
 _d_dprob = (dprob.crisp? dprob.crispvalue :dprob.op.defuz(dprob));
 return _d_dprob;
}

