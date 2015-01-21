#!/usr/bin/env python
# -*- coding: utf-8 -*-

import paramiko
import time
import re

server = "net1"
username = "root"
step = 0.1 # sec

sent = re.compile(r"Sent ([0-9]+) bytes", re.S)
packets = re.compile(r"bytes ([0-9]+) pkts", re.S)
dropped = re.compile(r"dropped ([0-9]+),", re.S)
marked = re.compile(r"marked ([0-9]+) early", re.S)
early = re.compile(r"early ([0-9]+) pdrop", re.S)
pdrop = re.compile(r"pdrop ([0-9]+) other", re.S)
q_error = re.compile(r"q_error ([0-9-]+) p_rate", re.S)
p_rate = re.compile(r"p_rate ([0-9]+) d_prob", re.S)
d_pdrop = re.compile(r"d_prob ([0-9-]+) prob", re.S)
prob = re.compile(r"prob ([0-9]+)\n", re.S)
backlog = re.compile(r"backlog ([0-9]+)b", re.S)

i = 0
header ="num;time;sent;packets;dropped;marked;early;pdrop;q_error;p_rate;d_pdrop;prob;backlog;\n"

final = ""
t = time.strftime("%m%d_%H-%M")
fname = "report_" + t + ".csv"
f = open(fname, "w")

f.write(header)

ssh = paramiko.SSHClient()
ssh.load_system_host_keys()
ssh.connect(server, key_filename = "/home/amasl/.ssh/id_rsa", look_for_keys = True, port = 8822, username=username) #, password=password)

time0 = time.time()

while True: 
    ssh_stdin, ssh_stdout, ssh_stderr = ssh.exec_command("tc -s qdisc ls dev wl0")
    a = ssh_stdout.readlines()
    out = ""
    for line in a:
        out = out + str(line)
    try:
        queue = backlog.findall(out)[0]
    except:
        queue = "N/A"
    tt = time.time() - time0

    final = "%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;\n" % (i,
                    tt,
                    sent.findall(out)[0],
                    packets.findall(out)[0],
                    dropped.findall(out)[0],
                    marked.findall(out)[0],
                    early.findall(out)[0],
                    pdrop.findall(out)[0],
                    q_error.findall(out)[0],
                    p_rate.findall(out)[0],
                    d_pdrop.findall(out)[0],
                    prob.findall(out)[0],
                    queue)  
    i = i + 1
    f.write(final)
    time.sleep(step)

f.close()
