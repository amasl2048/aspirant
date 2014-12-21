if {$argc == 2} {
    set dl(1) [lindex $argv 0]
    set bw(1) [lindex $argv 1]
}
puts $dl(1)
puts $bw(1)

set ns [new Simulator]
$ns use-scheduler Heap;                # use the Heap scheduler

set tracefile [open trace.log w]
set qsize     [open queuesize.log w]
set util      [open util.log w]
set qlost     [open queuelost.log w]
set param     [open parameters.log w]
$ns trace-all $tracefile

proc finish {} {
	global ns nf tracefile qsize util qlost bw Duration
	$ns flush-trace
	close $tracefile
	close $qsize
	close $util
	close $qlost
	exec perl ../throughput.pl trace.log 0 1 0.1 > trp.log
	exec perl ../throughput_ack.pl trace.log 4 0 0.1 > http_trp.log
	exec perl ../throughput_drop.pl trace.log 0 1 0.1 > drate.log
	exec mstd2 queuesize.log 3 > sci_results.txt
	exec mstd2 trp.log 3 > ftp_avg.txt
	exec mstd2 http_trp.log 3 > http_trp.txt
	exec mstd http.log 4 6 4 > http_avg.txt
	exec awk -v bw=$bw(1) -v t=$Duration -f ../util.awk trace.log > util.txt
	exec ../awk.sh > awk_results.txt
	exec ../plot.sh
	exec date > date.txt &
	exec ../trace.sh
	exec python ../dov_intervals.py queuesize.log > py_report.txt
	exit 0
}

# Link bandwidth Mbit/s
#set bw(1) 15

Queue/FLC2 set version_ 3
Queue/FLC2 set q_weight_ 1
Queue/FLC2 set s_ 10
Queue/FLC2 set ave_ 0
Queue/FLC2 set curq_ 0
Queue/FLC2 set qerror_norm_ 0
Queue/FLC2 set qerror_old_norm_ 0
Queue/FLC2 set p_rate_ 0
Queue/FLC2 set prob_ 0
Queue/FLC2 set d_prob_ 0
Queue/FLC2 set kf_ 0
### "true" - For ECN enable
Queue/FLC2 set setbit_ true
Queue/FLC2 set qref_ 300
Queue/FLC2 set max_p_ 0.1
Queue/FLC2 set d_scale_ 8e-5
Queue/FLC2 set mean_pktsize_ 1000
Queue/FLC2 set adaptive_ 0
Queue/FLC2 set sampling_ 0.006
Queue/FLC2 set lock_mode_ 0
### Bytes per second: 15Mbit/s*1000*1000/8bits
Queue/FLC2 set p_rate_0_ [expr $bw(1)*1000*1000/8]

# Bottelneck 0-1
set n0 [$ns node]
set n1 [$ns node]
$ns duplex-link $n0 $n1 $bw(1)Mb $dl(1)ms FLC2
$ns queue-limit $n0 $n1 500

# Trace P drop for FLC
set pdrop [open pdrop.log w]
set flcq [[$ns link $n0 $n1] queue]
$flcq trace prob_
$flcq trace d_prob_
$flcq trace qerror_norm_
$flcq trace p_rate_
#$flcq trace kf_
$flcq attach $pdrop

# Destination n2
set n2 [$ns node]
$ns duplex-link $n1 $n2 200Mb 5ms DropTail

### UDP Traffic ###################
# CBR direct
set n3 [$ns node]
$ns duplex-link $n3 $n0 100Mb 5ms DropTail
set udp [new Agent/UDP]
$ns attach-agent $n3 $udp
set null [new Agent/Null]
$ns attach-agent $n2 $null
$ns connect $udp $null
$udp set fid_ 1

set cbr [new Application/Traffic/CBR]
$cbr attach-agent $udp
$cbr set packetSize_ 1000
$cbr set rate_ 0.128Mb

# CBR reverse
set udp_r [new Agent/UDP]
$ns attach-agent $n2 $udp_r
set null_r [new Agent/Null]
$ns attach-agent $n3 $null_r
$ns connect $udp_r $null_r
$udp_r set fid_ 2

set cbr_r [new Application/Traffic/CBR]
$cbr_r attach-agent $udp_r
$cbr_r set packetSize_ 1000
$cbr_r set rate_ 0.128Mb
#####################################

### HTTP Traffic #################
set CLIENT 0
set SERVER 1

# SETUP TOPOLOGY
# create nodes
set n4 [$ns node]
# create link
$ns duplex-link $n4 $n0 100Mb 5ms DropTail

# SETUP PACKMIME
set rate 50;			# new connections per second
set pm [new PackMimeHTTP]
$pm set-client $n2;                  # name $n(0) as client
$pm set-server $n4 ;                 # name $n(1) as server
$pm set-rate $rate;                    # new connections per second
$pm set-http-1.1   ;                   # use HTTP/1.1
$pm set-TCP Newreno
Agent/TCP/FullTcp set segsize_ 960

# SETUP PACKMIME RANDOM VARIABLES

# create RNGs (appropriate RNG seeds are assigned automatically)
set flowRNG [new RNG]
set reqsizeRNG [new RNG]
set rspsizeRNG [new RNG]
$flowRNG seed 3
$reqsizeRNG seed 4
$rspsizeRNG seed 5

# create RandomVariables
set flow_arrive [new RandomVariable/PackMimeHTTPFlowArrive $rate]
set req_size [new RandomVariable/PackMimeHTTPFileSize $rate $CLIENT]
set rsp_size [new RandomVariable/PackMimeHTTPFileSize $rate $SERVER]

# assign RNGs to RandomVariables
$flow_arrive use-rng $flowRNG
$req_size use-rng $reqsizeRNG
$rsp_size use-rng $rspsizeRNG

# set PackMime variables
#Set the time between two consecutive connections starting
$pm set-flow_arrive $flow_arrive
#Set the HTTP request size distribution
$pm set-req_size $req_size
#Set the HTTP response size distribution
$pm set-rsp_size $rsp_size

# record HTTP statistics
$pm set-outfile "http.log"
###############################

# TCP Sources
set NumbSrc 100;

for {set j 1} {$j<=$NumbSrc} { incr j } {
set S($j) [$ns node]
}

# Random FTP start
set rng [new RNG]
$rng seed 2

set RVstart [new RandomVariable/Uniform]
$RVstart set min_ 1
$RVstart set max_ 9
$RVstart use-rng $rng

for {set i 1} {$i<=$NumbSrc} { incr i } {
#set startT($i) 0.0
set dly($i) [expr [$RVstart value]]
puts $param "dly($i) $dly($i) msec"
}

# Links
for {set j 1} {$j<=$NumbSrc} { incr j } {
$ns duplex-link $S($j) $n0 100Mb $dly($j)ms DropTail
}

# Enable ECN
Agent/TCP set ecn_ 1
Agent/TCPSink set ecn_syn_ true

# TCP Sources
for {set j 1} {$j<=$NumbSrc} { incr j } {
set tcp_src($j) [new Agent/TCP/Newreno]
$tcp_src($j) set window_ 8000
}

#TCP Dest
for {set j 1} {$j<=$NumbSrc} { incr j } {
set tcp_snk($j) [new Agent/TCPSink]
}

#Connections
for {set j 1} {$j<=$NumbSrc} { incr j } {
$ns attach-agent $S($j) $tcp_src($j)
$ns attach-agent $n2 $tcp_snk($j)
$ns connect $tcp_src($j) $tcp_snk($j)
}

# FTP Src
for {set j 1} {$j<=$NumbSrc} { incr j } {
set ftp($j) [$tcp_src($j) attach-source FTP]
}

for {set j 1} {$j<=$NumbSrc} { incr j } {
$tcp_src($j) set packetSize_ 960
}

### Queue monitoring

set qfile [$ns monitor-queue $n0 $n1 [open queue.log w] 0.1]
[$ns link $n0 $n1] queue-sample-timeout;
set parr_last 0.0
set pdrop_last 0.0

proc record {} {
global ns qfile qsize util qlost n0 n1 pdrop parr_last pdrop_last bw
set time 0.1
set loss 0.0
set now [$ns now]
$qfile instvar parrivals_ pdepartures_ pdrops_ bdepartures_ bdrops_
puts $qsize "$now [expr $parrivals_-$pdepartures_-$pdrops_]"
### Utilization at bw Mbps rate
puts $util "$now [expr 1.0*$bdepartures_*8/1000/1000/$bw(1)/$time]"
#puts $qbw "$now [expr $bdepartures_*8/1024/$time]"
set bdepartures_ 0
if { $parrivals_ == $parr_last} {
	 set loss 0.0
} else {
	set loss [expr abs(1.0*($pdrops_-$pdrop_last)/($parrivals_-$parr_last))]
}
puts $qlost "$now $loss"
set parr_last $parrivals_
set pdrop_last $pdrops_
$ns at [expr $now+$time] "record"
}

set Duration 600;
# Monitor
$ns at 0.0 "record"

# FTP Schedule
# Start
set startd 0

for {set i 1} {$i<=$NumbSrc} { incr i } {
$ns at 0 "$ftp($i) start"
}

for {set j 1} {$j<=[expr $Duration/100]} { incr j } {
# Half-stop
for {set i 51} {$i<=$NumbSrc} { incr i } {
$ns at [expr $startd+40] "$ftp($i) stop"
}
# Half-start
for {set i 51} {$i<=$NumbSrc} { incr i } {
$ns at [expr $startd+70] "$ftp($i) start"
}
set startd [expr $startd+100];            # Repeate test each 100 sec
}

# Stop
for {set i 1} {$i<=$NumbSrc} { incr i } {
$ns at $Duration "$ftp($i) stop"
}

# HTTP
$ns at 0 "$pm start"
$ns at $Duration "$pm stop"

# UDP
$ns at 0 "$cbr start"
$ns at 0 "$cbr_r start"
$ns at $Duration "$cbr stop"
$ns at $Duration "$cbr_r stop"

$ns at $Duration "finish"
$ns run
