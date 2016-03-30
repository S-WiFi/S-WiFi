# Copyright (c) 1997 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by the Computer Systems
#      Engineering Group at Lawrence Berkeley Laboratory.
# 4. Neither the name of the University nor of the Laboratory may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# simple-wireless.tcl
# A simple example for wireless simulation

# ======================================================================
# Handle command line arguments
# ======================================================================
proc usage {} {
	global argv0
	puts "$argv0 func mode"
	puts "    func    One of pcf (default), rtt, reliability, and delay"
	puts "    mode    One of baseline (default) or smart if func is pcf;"
	puts "            one of downlink (default) or uplink otherwise"
	exit 0
}
if {$argc < 1} {
	set func "pcf"
} else {
	set func [lindex $argv 0]
}
# Allow abbreviated command line arguments.
# e.g. `ns swifi.tcl d` is the same as `ns swifi.tcl delay`
switch -glob -nocase $func {
	d* {
		set func "delay"
	}
	re* {
		set func "reliability"
	}
	rt* {
		set func "rtt"
	}
	p* {
		set func "pcf"
	}
	default {
		usage
	}
}
if {$argc < 2} {
	if {0 == [string compare $func "pcf"]} {
		set mode "baseline"
	} else {
		set mode "downlink"
	}
} else {
	set mode [lindex $argv 1]
}
switch -glob -nocase $mode {
	d* {
		set mode "downlink"
	}
	u* {
		set mode "uplink"
	}
	b* {
		set mode "baseline"
	}
	s* {
		set mode "smart"
	}
	default {
		usage
	}
}
if {0 == [string compare $func "pcf"]} {
	if {0 == [string compare $mode "baseline"] || 0 == [string compare $mode "smart"]} {
		# Disable retry in MAC layer.
		set retry 0
	} else {
		usage
	}
} else  {
	  if {0 == [string compare $mode "uplink"] || 0 == [string compare $mode "downlink"]} {
		if {0 == [string compare $func "delay"]} {
			if {$argc < 3} {
				set retry 1
			} else {
				set retry [lindex $argv 2]
			}
		}
		else {
			set retry 1
	  	}
	  } else {
 		usage
	  }
}
if {[expr 0 == [string compare $func "pcf"] && 0 == [string compare $mode "smart"]]} {
	puts stderr "Unimplemented!"
	exit 1
}

puts "func: $func, mode: $mode"

# ======================================================================
# Define options
# ======================================================================

set val(chan)           Channel/WirelessChannel    ;# channel type
set val(prop)           Propagation/Shadowing      ;# radio-propagation model
set val(netif)          Phy/WirelessPhy            ;# network interface type
set val(mac)            Mac/802_11                 ;# MAC type
set val(ifq)            Queue/DropTail/PriQueue    ;# interface queue type
set val(ll)             LL                         ;# link layer type
set val(ant)            Antenna/OmniAntenna        ;# antenna model
set val(ifqlen)         50                         ;# max packet in ifq
if {0 == [string compare $func "pcf"]} {
	set val(nn)             3                  ;# number of mobilenodes
} else {
	set val(nn)             2                  ;# number of mobilenodes
}
set val(rp)             DumbAgent                  ;# routing protocol


# ======================================================================
# Main Program
# ======================================================================


# ======================================================================
# Initialize Global Variables
# ======================================================================

set ns_		[new Simulator]
set tracefname  [format "swifi_%s_%s.tr" $func $mode]
set tracefd     [open $tracefname w]
$ns_ trace-all $tracefd

# set up topography object
set topo       [new Topography]

$topo load_flatgrid 500 500

# ======================================================================
# Create God (General Operations Director)
# ======================================================================

create-god $val(nn)

# ======================================================================
# Configure node
# ======================================================================

Phy/WirelessPhy set Pt_ 1
Propagation/Shadowing set pathlossExp_ 2.0  ;# path loss exponent
Propagation/Shadowing set std_db_ 4.0       ;# shadowing deviation (dB)
Propagation/Shadowing set dist0_ 1.0        ;# reference distance (m)
Propagation/Shadowing set seed_ 0           ;# seed for RNG

Mac/802_11 set dataRate_  11.0e6
Mac/802_11 set basicRate_ 1.0e6
Mac/802_11 set CWMin_         1
Mac/802_11 set CWMax_         1
Mac/802_11 set PreambleLength_  144                   ;# long preamble 
Mac/802_11 set RTSThreshold_  5000
Mac/802_11 set PLCPDataRate_  1.0e6                   ;# 1Mbps
Mac/802_11 set ShortRetryLimit_  [expr $retry + 1]    ;# retransmittions
Mac/802_11 set LongRetryLimit_   [expr $retry + 1]    ;# retransmissions
Mac/802_11 set TxFeedback_ 0;

Agent/SWiFi set packet_size_ 1000
#Agent/SWiFi set slot_interval_ 0.01
if {0 == [string compare $mode "smart"]} {
	Agent/SWiFi set pcf_policy_ 1
}
Agent/SWiFi set realtime_ true

set logfname [format "swifi_%s_%s.log" $func $mode]
set logf [open $logfname w]
set datfname [format "swifi_%s_%s.dat" $func $mode]
set datf [open $datfname w]
set logqname [format "swifi_%s_%s_queue.log" $func $mode]
set logq [open $logqname w]
set loganame [format "swifi_%s_%s_arrival.log" $func $mode]
set loga [open $loganame w]
if {0 == [string compare $func "delay"]} {
	set delayfname [format "swifi_%s_%s_%d.dat" $func $mode $retry]
	set delayf [open $delayfname w]
}
set n_rx 0
Agent/SWiFi instproc recv {from rtt data} {
	global logf delayf n_rx func
	set n_rx [expr $n_rx + 1]
        $self instvar node_
	if {0 != [string compare $func "delay"]} {
		set rtt_name "round-trip-time"
	} else {
		set rtt_name "delay"
	}
        puts $logf "Node [$node_ id] received reply from node $from\
		with $rtt_name $rtt ms and message $data."
	if {0 == [string compare $func "delay"]} {
		puts $delayf "$rtt"
	}
	flush $logf
}
Agent/SWiFi instproc stat {n_run} {
	global n_rx num_trans distance reliability datf interval func
	set reliability($n_run) [expr double($n_rx) * $interval / $num_trans]
	if {0 == [string compare $func "pcf"]} {
		puts $datf "$reliability($n_run)"
	} else {
		puts $datf "$distance($n_run) $reliability($n_run)"
	}
	flush $datf
	set n_rx 0
}
Agent/SWiFi instproc alog { num } {
	global loga
	$self instvar node_
	puts $loga "Node [$node_ id] current number of data packets = $num"
	flush $loga
}
proc qlog { node qlen } {
	global logq
	puts $logq "Node $node current queue length = $qlen"
	flush $logq
}

set dRNG [new RNG]
$dRNG seed [lindex $argv 0]
$dRNG default

# Create channel
# cf. ns-2.35/tcl/ex/wireless-mitf.tcl
set chan_1_ [new $val(chan)]

$ns_ node-config -adhocRouting $val(rp) \
				 -llType $val(ll) \
			 	 -macType $val(mac) \
			 	 -ifqType $val(ifq) \
			 	 -ifqLen $val(ifqlen) \
			 	 -antType $val(ant) \
			 	 -propType $val(prop) \
			 	 -phyType $val(netif) \
			 	 -channel $chan_1_ \
			 	 -topoInstance $topo \
			 	 -agentTrace ON\
			 	 -routerTrace OFF \
			 	 -macTrace ON \
			 	 -movementTrace OFF			

# ======================================================================
# Create the specified number of mobilenodes [$val(nn)] and "attach" them
# to the channel. 
# Here two nodes are created : node(0) and node(1)
# ======================================================================

set node_(0) [$ns_ node]
$node_(0) set X_ 3
$node_(0) set Y_ 100
$node_(0) set Z_ 0
set sw_(0) [new Agent/SWiFi]
$ns_ attach-agent $node_(0) $sw_(0)

if {0 != [string compare $func "delay"]} {
	# FIXME better way to specify distances
	set distance(0) 1
	set distance(1) 100
	set distance(2) 200
} else {
	# Set the distance that the reliability is >= 55% per Problem 3.
	set distance(0) 1000
}

# Build a LUT of (distance, reliability).
set lutfp [open "report/swifi_reliability_uplink.dat" r]
set lutfile [read $lutfp]
close $lutfp
set pattern {([\.0-9]+)\s+([\.0-9]+)}
foreach {fullmatch m1 m2} [regexp -all -line -inline $pattern $lutfile] {
	set lut($m1) $m2
}

for {set i 1} {$i < $val(nn) } {incr i} {
	set node_($i) [$ns_ node]	
	$node_($i) random-motion 0		;# disable random motion
	$node_($i) set X_ [expr 3.0 + $distance($i)]
	$node_($i) set Y_ 100
	$node_($i) set Z_ 0
	set sw_($i) [new Agent/SWiFi]
	$ns_ attach-agent $node_($i) $sw_($i)
}

# ======================================================================
# Specify events
# ======================================================================

set mymac [$node_(0) set mac_(0)]
$ns_ at 0.0 "$sw_(0) mac $mymac"
$ns_ at 0.5 "$sw_(0) server"
#$mymac setTxFeedback 1

for {set i 1} {$i < $val(nn)} {incr i} {
	$ns_ connect $sw_($i) $sw_(0)
	set cmd "$sw_($i) register $lut($distance($i)) 0 0 0"
	#puts "register cmd: $cmd"
	$ns_ at [expr 3.0 + 0.1*$i] $cmd
}

set period     100.0
if {0 == [string compare $func "reliability"]} {
	set num_runs   21
	set delta_dist 100
} elseif {0 == [string compare $func "pcf"]} {
	set num_runs   10
} else {
	set num_runs   1
}
set num_trans  10000
if {0 != [string compare $func "delay"]} {
	set slot 0.01
} else {
	# RTT is acquired from measurements in Problem 1&2.
	set rtt 0.001625
	set slot [expr 2 * $rtt]
}
# specify the number of slots in an interval
set interval 10
set rand_min 0
set rand_max 2


proc rand_int { min max } {
	return [expr {int(rand()*($max-$min+1) + $min)}]
}

if {0 != [string compare $mode "downlink"]} {
	set command "$sw_(0) poll"
} else {
	set command "$sw_(0) send"
}
for {set k 0} {$k < $num_runs} {incr k} {
	if {0 == [string compare $func "reliability"] && [expr $k > 0]} {
		for {set i 1} {$i < $val(nn)} {incr i} {
			set distance($k) [expr $delta_dist * $k]
			$ns_ at [expr $period*($k + 1) - 0.002] \
				"$node_($i) set X_ [expr 3.0 + $distance($k)]"
		}
	}
	if {[expr $k > 0]} {
		for {set i 0} {$i < $val(nn)} {incr i} {
			$ns_ at [expr $period*($k + 1) - 0.001] "$sw_($i) restart"
		}
	}
	for {set i 0} {$i < $num_trans} {incr i} {
		$ns_ at [expr $period * ($k + 1) + $i * $slot] "$command"
		if { $i % $interval == 0} {
			# boi = beginning of interval
			$ns_ at [expr $period * ($k + 1) + $i * $slot - 0.0002] "$sw_(0) boi"
			for {set j 1} {$j < $val(nn)} {incr j} {
				set rand_val [rand_int $rand_min $rand_max]
				$ns_ at [expr $period * ($k + 1) + $i * $slot - 0.0001] "$sw_($j) pour $rand_val"
			}
		}
	}
	$ns_ at [expr $period*($k + 2) - 0.003] "$sw_(0) stat $k"
}

#$ns_ at 8000.0 "$sw_(0) report" 

$ns_ at 10000.0 "stop"
$ns_ at 10000.01 "puts \"NS EXITING...\" ; $ns_ halt"

#
#Mac/802_11 instproc txfailed {} {
#	upvar sw_(0) mysw
#	$mysw update_failed 
#}

#Mac/802_11 instproc txsucceed {} {
#	upvar sw_(0) mysw
#	$mysw update_delivered 
#}

#Mac/802_11 instproc brdsucced {} {
#}

proc stop {} {
	global ns_ tracefd logf
	$ns_ flush-trace
	close $tracefd
	close $logf
}


puts "Starting simulation..."
$ns_ run
