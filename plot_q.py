#!/usr/bin/env python
# -*- coding: utf-8 -*-

#from numpy import
from numpy import loadtxt, savetxt, size, mean, std, array
import matplotlib.pyplot as mpl
#import csv

import sys, os, csv, re
try:
    fname = str(sys.argv[1])
#    print fname
except:  # ValueError:
    print 'Please supply file name\n'
    print 'Usage: plot_q.py <filename>'

# try:
# 	s = int(sys.argv[4])
# except:
# 	s = 0

f = open(sys.argv[1], 'r')
#f2 = open("out.txt", 'w')
p = re.compile(r"\[(.*?)\]", re.S)
b = []
for j in f.readlines():
    #t.append(p.findall(j)[0] + "\n")
    b.append(float((p.findall(j)[0].strip())))
#f2.writelines(t)
f.close()
#f2.close()
t = array(b)
t = t - t[0]

a = loadtxt(fname,
               #dtype={"names": ('time', 'val'),
               #             "formats": ('S16', 'S16')},
            #delimiter=" ", usecols=(7, 9, 11, 13, 15), unpack=False)
            delimiter=" ", usecols=(8, 10, 12, 14, 16), unpack=False)
            #delimiter=" ", usecols=(9, 11, 13, 15, 17), unpack=False)

n=0            
x = a[:,n]
print "min: ", min(x), "\t max: ", max(x)
print "size: ", size(x), "\t mean: ", mean(x), "\t std:", std(x)

xfile = "q_" + fname[-1:]
# f = open(xfile, "w")
# tmp_out = sys.stdout
# sys.stdin = f
# print "%i" % x
# sys.stdout = tmp_out
z = zip(t,x)
savetxt(xfile, z)

mpl.figure(1)
mpl.plot(t, x, "k-")  #, "b.", markersize=3)
#mpl.title(u"Длина очереди")
mpl.xlabel(u"Время, с")
mpl.ylabel(u"Длина очереди, байт")
mpl.plot([0,400], [250000,250000], "r--")
mpl.ylim(0,500000)
mpl.savefig("q_len.png")

## mpl.figure(2)
## mpl.plot(t, a[:, n])  #, "r.", markersize=3)
## #legend( ("X","Y"), "upper right" )
## mpl.title("q_error")

## mpl.figure(3)
## mpl.plot(t, a[:, n+1])  #, "r.", markersize=3)
## mpl.title(u"Относительная скорость")

## mpl.figure(4)
## mpl.plot(t, a[:, n+2])  #, "r.", markersize=3)
## mpl.title(u"dprob")

## mpl.figure(5)
## mpl.plot(t, a[:, n+3])  #, "r.", markersize=3)
## mpl.title(u"Вероятность сброса")

#mpl.show()
