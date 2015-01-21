#!/usr/bin/env python
# -*- coding: utf-8 -*-

import numpy as np
import pandas as pd
import matplotlib as mpl
import matplotlib.pyplot as plt

mpl.rcParams["font.family"] = "serif"
mpl.rcParams["font.size"] = 18

df=pd.read_csv('report_1220_22-00.csv', sep=';', header=0, index_col=None)

plt.plot(df['time'], df['backlog'])
plt.xlabel(u"Время, сек.")
plt.ylabel(u"Длина очереди, байты")

print "mean: ", df['backlog'].mean()
print "std: ", df['backlog'].std()

s = np.size(df['packets'])
print "ploss:  ", df['dropped'][s-1] / float(df['packets'][s-1]) * 100

