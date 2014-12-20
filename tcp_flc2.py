#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Calcucation of TCP flow

Author:  A.Maslennikov
2013 
"""

from numpy import *
import pylab as py
from flc_lib64 import flc

k = 3
# Definitions of parameters
wmax = 8000
rtt = 0.01 # RTT time sec
c = 100. # link rate segments/sec
qref = 25 # queue reference
qmax = 50
tmax = 20.
h = 0.005 # t step
numb = tmax/h
delay = rtt/h # digital delay
p_last=0
method = ("dt","red","flc")
mtd = method[2]
print mtd

def ind(x):
	'''
	indicator function
	'''
	if (x>0): f=1
	else: f=0
	return f

def ed(x):
	'''
	1 function
	'''
	f=x
	if (x>1): f=1
	if (x<0): f=0
	return f

def queue(x):
	'''
	Max.limit, non zero
	'''
	f=x
	if (x>qmax): f=qmax
	if (x<0): f=0
	return f

def win(x):
	'''
	TCP window - Max.limit, non zero
	'''
	f=x
	if (x>wmax): f=wmax
	if (x<0): f=0
	return f

def prob(q, avg, wn, qn):
    '''
    Probability
    RED method
    '''
    if (mtd == "dt"): # dt
		if (q < qmax-1 ): p=0
		if (q >= qmax-1): p=1
#		
    if (mtd == "red"): # red
		minth = qmax/3
		maxth = qmax
		maxp = 0.1
		p = maxp*(avg-minth)/(maxth-minth)
		if (p<0): p=0
		if (avg<=minth): p=0
		if (avg>=maxth): p=1
#		
    if (mtd == "flc"): # flc
        q_norm = (q-qref)/qmax
        rate_norm = 0
        rate_norm = (wn/t_t(qn)-c)/c
        if (rate_norm > 1): rate_norm=1
        p_flc = flc.flc_003IE(q_norm, rate_norm)
        p_last = p_flc
        p = p_last + p_flc * 0.000085
        if (p<0): p=0
            
        
    return p

def c_t(q):
	"""
	Link rate
	"""
#	c = 100.0 # link rate
	if (q>c):
		return c
	if (q<=c):
		return q


def t_t(q):
	"""
	Time delay
	"""
    
	if (q<=0):
		return rtt
	if (q>0):
		return rtt + q/c


def dw_dt(t, w, q, p):
	tt = t_t(q)
	w1 = ind(wmax-w) / tt
	w2 = ind(w-1) * w * w * p / ( 2 * tt )
	dw = w1 - w2
    
	return dw

def dq_dt(t, w, q, p):
	ntcp = 1 # TCP flows
	qq1 = ntcp * ind(qmax-q) * w * (1-p) / t_t(q)
	qq2 = c
	dq = qq1 - qq2
#	
	return dq

def rk4(f, g, X0, h):
	Xa = zeros(numb) # time
	Ya = zeros(numb) # TCP window
	Za = zeros(numb) # Queue size
	p =  zeros(numb) # Drop Probability
	avg = zeros(numb+1)
	n = 0
	Xo, Yo, Zo = X0
	wq = 0.002

	while ( Xo < tmax ):

		Xa[n] = Xo
		Ya[n] = Yo
		Za[n] = Zo

		p[n] = prob(Zo, avg[n], Ya[n], Za[n])

		k1 = h*f(Xo, Yo, Zo, p[n])
		q1 = h*g(Xo, Yo, Zo, p[n])
	#	
		k2 = h*f(Xo + h/2.0, Yo + k1/2.0, Zo + k1/2.0, p[n])
		q2 = h*g(Xo + h/2.0, Yo + q1/2.0, Zo + q1/2.0, p[n])

		k3 = h*f(Xo + h/2.0, Yo + k2/2.0, Zo + k2/2.0, p[n])
		q3 = h*g(Xo + h/2.0, Yo + q2/2.0, Zo + q2/2.0, p[n])

		k4 = h*f(Xo + h, Yo + k3, Zo + k3, p[n])
		q4 = h*g(Xo + h, Yo + q3, Zo + q3, p[n])

		Y1 = Yo + (1/6.0)*(k1 + 2.0*k2 + 2.0*k3 + k4)
		Z1 = Zo + (1/6.0)*(q1 + 2.0*q2 + 2.0*q3 + q4)
#	

		Xo += h
		Yo = win(Y1)
		Zo = queue(Z1)
		n += 1
		avg[n] = (1-wq) * avg[n-1] + wq * Zo # exponential weighted moving average (EWMA)

	return Ya, Za, p, avg

X0 = array([0, 0, 0])                   # initials conditions
w, q, pr, avg = rk4(dw_dt, dq_dt, X0, h)

'''
Plot
'''
t = linspace(0, tmax, numb)           # time
f1 = py.figure(1)

py.subplot(3,1,1)
#py.legend(loc='best')
py.title('TCP congestion window size W(t)')
py.ylabel('segments')
py.plot(t, w, 'k-')

py.subplot(3,1,2)
#py.grid()
py.title('Queue size Q(t)')
py.ylabel('packets')
py.plot(t, q, 'k-')

#py.subplot(4,1,3)
#py.grid()
#py.title('queue average')
#py.plot(t, avg[:numb], 'k-')

py.subplot(3,1,3)
#py.grid()
py.title('Drop probability P(t)')
py.plot(t, pr, 'k-')

py.xlabel('time, sec')
f1.savefig('flc2.png')

#py.show()

savetxt("t.log",t)
savetxt("w.log",w)
savetxt("q.log",q)
savetxt("pr.log",pr)
