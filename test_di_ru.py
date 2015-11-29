#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
 Графики jitter, pdrop с доверительными интервалами
"""
import yaml
from matplotlib.pyplot import *
from matplotlib import rc

#rc('font', family = 'serif')
#rc('font', serif = 'Times New Roman') 
rc('font', size = 24) 

f = open('results.yml')
data = yaml.safe_load(f)
f.close()

key = data.keys()
jitter = [data[x]["jitter"] for x in key]
jitter_int = [data[x]["jitter_int"] for x in key]
pdrop = [data[x]["pdrop"] for x in key]
pdrop_int = [data[x]["pdrop_int"] for x in key]

n = len(key)
x = range(n)

fig1 = figure(1)
#subplot(2,1,1)
bar(x, jitter, yerr=jitter_int, align="center", color="0.9", ecolor="k")
xticks(x, ["FLC", "Tail Drop", "RED"])
#plot([0,n], [250,250], "r--")
#plot([0,n], [500,500], "r--")
#plot([0,n], [1000,1000], "r--")
#title(u"Jitter")
xlabel(u"Метод")
ylabel(u"Джиттер, мс")
xlim(-0.5, 2.5)
#annotate(u'FLC', xy=(0.5, 4))
#annotate(u'TailDrop', xy=(1.5, 6))
#annotate(u'RED', xy=(2.5, 8))
fig1.savefig("jitter.pdf")

fig2 = figure(2)
#subplot(2,1,2)
bar(x, pdrop, yerr=pdrop_int, align="center", color="0.9", ecolor="k")
xticks(x, ["FLC", "Tail Drop", "RED"])
#title(u"Packets loss")
xlabel(u"Метод")
ylabel(u"Потери пакетов, %")
xlim(-0.5, 2.5)
#annotate(u'FLC', xy=(0.5, 0.2))
#annotate(u'TailDrop', xy=(1.5, 1))
#annotate(u'RED', xy=(2.5, 0.2))
fig2.savefig("ploss.pdf")
