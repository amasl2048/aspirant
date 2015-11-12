# -*- coding: utf-8 -*-
import yaml
import matplotlib.pyplot as plt
import numpy as np
import matplotlib as mpl
mpl.rcParams["font.family"] = "serif"
mpl.rcParams["font.size"] = 18

mu = 10.
num = 50

f1 = yaml.load(open("run4.yaml"))
f2 = yaml.load(open("flc2.yaml"))
sur = yaml.load(open("sur8.yaml"))

name1 = "common"
n = np.size(f1[name1])
n2 = np.size(f2[name1])
ns = np.size(sur)

la = np.zeros(n)
me = np.zeros(n)
lo = np.zeros(n)
la2 = np.zeros(n2)
me2 = np.zeros(n2)
lo2 = np.zeros(n2)
rate = np.zeros(ns)
qmean = np.zeros(ns)

for i in np.arange(n):
    la[i] = f1[name1][i]["lambda"]
    me[i] = f1[name1][i]["mean"]
    lo[i] = f1[name1][i]["pkt_loss"]

for i in np.arange(n2):   
    la2[i] = f2[name1][i]["lambda"]
    me2[i] = f2[name1][i]["mean"]
    lo2[i] = f2[name1][i]["pkt_loss"]

for i in np.arange(ns):
    rate[i] = str(sur[i]["in_rate"])
    qmean[i] = str(sur[i]["queue mean size"])

ro = np.zeros(n)
ro = la2/mu # la
pb = (1-ro)*pow(ro, num)/(1-pow(ro, num+1))
navg = ro/(1-ro) - (num+1)*pow(ro, num+1)/(1-pow(ro, num+1))

plt.figure(1)
#py.title(u'Средняя длина очереди')
plt.xlabel(ur'Нагрузка , $\rho$')
plt.ylabel(u'Средняя длина очереди')
plt.plot(la2/mu, navg, "o-", label=u"расчёт системы M/M/1/50 TD") #la
plt.plot(rate, qmean, "kx", label=u"математическая модель FLC")
plt.plot(la2/mu, me2, "o--", label=u"имитационная модель FLC")
#plt.plot(la/mu, me, "o-.", label=u"моделирование DropTail")
plt.legend(loc="lower right")
plt.savefig("mm1.pdf")

plt.show()
