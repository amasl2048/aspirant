#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
 student.py
 Оченка доверительного интервала
 t-статистика Стьюдента

"""
from numpy import array, mean, std, sqrt, size
from scipy import stats
import yaml

f = open('test_iperf.yml')
data =  yaml.safe_load(f)
#mm = array([323.49, 322.066, 322.197, 321.874, 322.583]) # список средних значений
f.close()

def student(mm):
    ''' Распределение Стьюдента '''
    alfa=0.05              # уровень значимости
    n = size(mm) - 1            # число степеней свободы
    t = stats.t(n)
    tcr = t.ppf(1 - alfa / 2)
    return round(mean(mm), 4), round (tcr * std(mm) / sqrt( size(mm) ), 4)
# Оценка
#print "среднее: ", mm
#print "<queue_mean>", mean(mm), "</queue_mean>"
#print "<queue_di>", tcr*std(mm)/sqrt(mm.size), "</queue_di>"

print "---"
for k in data.keys():
    print "%s:" % k
    print "  jitter: %s\n  jitter_int: %s" % student(data[k]["jitter"])
    pd = []
    for dr in data[k]["dropped"]:
        for st in data[k]["sent"]:
            pd.append(float(dr) / float(st) * 100. ) 
    print "  pdrop: %s\n  pdrop_int: %s" % student(pd)



