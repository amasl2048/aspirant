#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Calcucation of stationary probability
 - Get in_rate value from cli

Author:  A.Maslennikov
2013 July
"""

from numpy import *
from matplotlib.pyplot import *
import sys
import matplotlib as mpl

try:
    rate = str(sys.argv[1])
    print "- in_rate: ", rate
except ValueError:
    print 'Please supply file name'

# Given
b = 50 # max buffer
qref = 25
delta = 0.2
l = int((1 - delta) * qref) # low lever
h = int((1 + delta) * qref) # hi level
st1 = "L=%s, Qref=%s, H=%s, B=%s" % (l, qref, h, b)
print "  buffer: ", st1
f = 9 # states
r0 = float(rate)
rn = array([r0, 1.2, .47, .43, .3, 0]) # rates
st2 = "  rates: %s" % rn
print st2
mu = 1.

# Ranges
xl = array([
	[0, qref],
	[1, qref+1],
	[l, h],
	[l+1, h+1],
	[qref,b-4],
	[qref+1, b-3],
	[h, b-2],
	[h+1, b-1],
	[b-1, b]
	])

# Good ranges - 6
xgood = array([
	[l, qref], #0
	[l, qref+1],
	[l, h], #2
	[l+1, h],
	[qref, h], #4
	[qref+1, h] #5
	])

# Number of conditions for each state
kn = zeros(f, dtype=int)
for i in range(f):
	kn[i] = int(xl[i, 1] - xl[i, 0] + 1)

sn = sum(kn) # whole number of conditions
st3 = "  states: %s" % sn
print st3

def func_r(i):
	"""
	Calculate rate index from condition number
	"""
	if (i==0): r = 0
	elif (i==1) or (i==2): r=1
	elif (i==3) or (i==4): r=2
	elif (i==5) or (i==6): r=3
	elif (i==7): r=4
	elif (i==8): r=5 # zero
	return r

def to_end(i):
	"""
	True - in case the system pass to the same status "s"
	"""
	if (i==0) or (i==2) or (i==4) or (i==6): return True
	if (i==1) or (i==3) or (i==5) or (i==7): return False

def x_index(i, x):
	"""
	Calculate A matrix global indexes
	form i,j states and local x,y indexes
	"""
	n = 0
	for k in range(i):
		n = n + kn[k]
	x0 = n + x
	return int(x0)

def y_index(j, y):
	"""
	Calculate A matrix global indexes
	form i,j states and local x,y indexes
	"""
	m = 0
	for k in range(j):
		m = m + kn[k]
	y0 = m + y
	return int(y0)

def queue(i, x):
	"""
    curent queue size
    i - current state 0..8
    x - index inside state
    """
	q = xl[i, 0] + x
	return q

A = zeros((sn, sn))
b = zeros(sn)
b[sn-1] = 1

for i in range(f):
	for j in range(f):
		if (i==j): # diag submatrix
			for x in range(kn[i]):
				for y in range(kn[j]):
					if (x==y): # diag elements
						r = func_r(i)
						x0 = x_index(i, x)
						y0 = y_index(j, y)
						A[x0, y0] = (-rn[r]-mu)
						if (i==0) and (x==0):
							A[x0, y0] = (-rn[r])
						if (i==8):
							A[x0, y0] = (-mu)
					if (x == (y-1)): # upper diag
						r = func_r(i)
						x0 = x_index(i, x)
						y0 = y_index(j, y)
						A[x0, y0] = rn[r]
					if (x == (y+1)): # lower diag
						r = func_r(i)
						x0 = x_index(i, x)
						y0 = y_index(j, y)
						A[x0, y0] = mu
		if (i == (j-1)) and (i != 8): # upper submatrix
			r = func_r(i)
			x = kn[i]-1
			x0 = x_index(i, x)
			if to_end(i): # pass to end
				y = kn[j]-1 # last element
				y0 = y_index(j, y)
				A[x0, y0] = rn[r]
				
			else: # pass to middle
				delta = xl[i, 1]-xl[i+1, 0] # current end minus next start
				y = delta + 1
				y0 = y_index(j, y)
				A[x0, y0] = rn[r]
				
		if (i == (j+1)) and (i != 0): # lower submatrix
			x = 0
			x0 = x_index(i, x)
			if not(to_end(i)): # pass to end
				y = 0 # first element
				y0 = y_index(j, y)
				A[x0, y0] = mu
				
			else: # pass to middle
				delta = xl[i, 0]-xl[i-1, 0] # current start minus previus start
				y = delta - 1
				y0 = y_index(j, y)
				A[x0, y0] = mu
				

#print sum(A,axis=1)
if (sum(sum(A, axis=1)) < 1e-14): print "  rows sum: OK"
#savetxt("a0.txt",A,fmt="%1.1f")

# last row with ones
AN = ones(sn)
AT = A.T
AT[sn-1, :] = AN

#savetxt("at.txt",AT,fmt="%1.1f")

con =  linalg.cond(AT)
dt =  linalg.det(AT)
st4 = "  condition number: %s" % int(con)
print st4
print "  determinant: ", dt

res = 0
res = linalg.solve(AT, b)
print "  prob. sum: ", sum(res), "  #(should be 1)"
if (sum(dot(res, AT)-b) < 1e-14): print "  test result: OK"

p = zeros(f)
m = zeros(f)
d = zeros(f)
for i in range(f): # states
	for x in range(kn[i]): # range in the i-state
		x0 = x_index(i, x)  # global index
		p[i] = p[i] + res[x0]
		m[i] = m[i] + queue(i, x) * res[x0]
		d[i] = d[i] + queue(i, x)**2 * res[x0] - (queue(i, x) * res[x0])**2
		
print "  probability: ", p
print "  queue mean size: ", sum(m)
#print "dispersion\t", d
print "  stdev: ", sqrt(sum(d))

pgood = zeros(f-3)
for i in range(f-3): # states
    for x in range(xgood[i, 0]-xl[i, 0], kn[i]-(xl[i, 1]-xgood[i, 1])): # range in the i-state
        x0 = x_index(i, x)  # global index
        pgood[i] = pgood[i] + res[x0]
        #        print i, x, x0, res[x0], pgood[i]

print "  prob_good: ", sum(pgood)

if ( (res[0] - 1./sn) < 1.e-8): print "#TRIVIAL"
else: print "#!!!"

mpl.rcParams["font.family"] = "serif"
mpl.rcParams["font.size"] = 14

figure(1)
bar(arange(f), p, color="black")
axis([0, f-1, 0, 1])
xlabel(u"Subset number")
ylabel(u"Probability")
st = st1 + "\n" + st2 + "\n" + st3 + "\n" + st4
#text(f/4, 0.5, st)
str2 = "states_" + str(r0) + ".png"
savefig(str2, dpi=300)

