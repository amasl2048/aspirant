#!/usr/bin/env python
# -*- coding: utf-8 -*-
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt

mpl.rcParams["font.family"] = "serif"
mpl.rcParams["font.size"] = 32

fname1 = "flc2_prob.log"
fname2 = "queuesize.log"
s = 0

prob = np.loadtxt(fname1, skiprows=s)
time1 = prob[:, 0]
pr = prob[:, 1]

queue = np.loadtxt(fname2, skiprows=s)
time2 = queue[:, 0]
qs = queue[:, 1]

plt.figure(1)
#errorbar(rate, qmean, yerr=std, fmt="o")
plt.plot(time1, pr, "k-")
#title(u"Средняя длина очереди")
plt.xlabel(u"Время, с")
plt.ylabel(u"Вероятность")
#plt.savefig("dprob_bw_s.pdf")

plt.figure(2)
#errorbar(rate, qmean, yerr=std, fmt="o")
plt.plot(time2, qs, "k-")
#title(u"Средне-квадратичное отклонение (СКО)")
plt.xlabel(u"Время, с")
plt.ylabel(u"Размер очереди, пакеты")
#plt.savefig("qsize_bw_s.pdf")

plt.show()
