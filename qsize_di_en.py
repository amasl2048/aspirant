# -*- coding: utf-8 -*-
"""
 Графики queue size с доверительными интервалами
 
"""
import matplotlib.pyplot as plt
import numpy as np
import matplotlib as mpl
#mpl.rcParams["font.family"] = "serif"
#mpl.rcParams["font.size"] = 16

n = 8
x = range (0, n)
fname = "table_bw50.txt"

#vnames = ("qmax", "qtar",
#          "queue_mean", "qm_di",
#          "queue_std", "qs_di")
#vformats = ("i4", "i3",
#            "f8", "f8",
#            "f8", "f8")
a = np.loadtxt(fname, #dtype={'names': vnames, 'formats':vformats},
        skiprows=1, usecols=(2,3,4,5), unpack=True )

plt.figure(1)
plt.subplot(2,1,1)
plt.bar(x, a[0], yerr=a[1], color="0.9", ecolor="k")
plt.plot([0,n], [300,300], "r--")
#plot([0,n], [500,500], "r--")
#plot([0,n], [1000,1000], "r--")
plt.title(u"Средняя длина очереди")
plt.xlabel(u"Методы")
plt.ylabel(u"Пакеты")
plt.xlim(0,n)
plt.annotate(u'ARED', xy=(0.3, 350))
plt.annotate(u'AVQ', xy=(1.3, 350))
plt.annotate(u'TD', xy=(2.3, 400))
plt.annotate(u'FEM', xy=(3.3, 350))
plt.annotate(u'FLC2', xy=(4.3, 350))
plt.annotate(u'PI', xy=(5.3, 350))
plt.annotate(u'RED', xy=(6.3, 350))
plt.annotate(u'REM', xy=(7.3, 350))


plt.subplot(2,1,2)
plt.bar(x, a[2], yerr=a[3], color="0.9", ecolor="k")
#plt.plot([0,n], [32,32], "r--")
plt.title(u"Среднеквадратичное отклонение")
plt.xlabel(u"Методы")
plt.ylabel(u"Пакеты")
plt.xlim(0,n)
plt.annotate(u'ARED', xy=(0.3, 50))
plt.annotate(u'AVQ', xy=(1.3, 50))
plt.annotate(u'TD', xy=(2.3, 50))
plt.annotate(u'FEM', xy=(3.3, 50))
plt.annotate(u'FLC2', xy=(4.3, 50))
plt.annotate(u'PI', xy=(5.3, 50))
plt.annotate(u'RED', xy=(6.3, 50))
plt.annotate(u'REM', xy=(7.3, 70))
plt.savefig("bars.pdf")
plt.show()

