/*
 * swifi.cc
 * Copyright (C) 2016 by Ping-Chun Hsieh, Tao Zhao
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */
/*
 * File: Code for downlink and uplink transmissions using S-WiFi
 * Author: Ping-Chun Hsieh (2016/02/17)
 */

#include "swifi.h"
#include <cstdlib>
#include <cstring>

int hdr_swifi::offset_;
static class SWiFiHeaderClass : public PacketHeaderClass {
public:
	SWiFiHeaderClass() : PacketHeaderClass("PacketHeader/SWiFi", 
					      sizeof(hdr_swifi)) {
		bind_offset(&hdr_swifi::offset_);
	}
} class_swifihdr;


static class SWiFiClass : public TclClass {
public:
	SWiFiClass() : TclClass("Agent/SWiFi") {}
	TclObject* create(int, const char*const*) {
		return (new SWiFiAgent());
	}
} class_swifi;


SWiFiClient::SWiFiClient() : addr_(0), is_active_(true)
{
	pn_ = 0;
	qn_ = 0;
	init_ = 0;
	tier_ = 1;
	exp_pkt_id_ = 0;
	num_data_pkt_ = 0;
	queue_length_ = 0;
}

SWiFiAgent::SWiFiAgent() : Agent(PT_SWiFi), seq_(0), mac_(0)
{
	num_client_ = 0;
	client_list_ = vector<SWiFiClient*>();
	is_server_ = 0;
	target_ = NULL;
	num_data_pkt_ = 0;
	poll_state_ = SWiFi_POLL_NONE;
	bind("packet_size_", &size_);
	bind("slot_interval_", &slot_interval_);
	bind_bool("do_poll_num_", &do_poll_num_);
	bind("pcf_policy_", &pcf_policy_);
	// Configure pcf features according to the bits of pcf_policy_.
	selective_ = ((pcf_policy_ & SWIFI_PCF_SELECTIVE) != 0);
	piggyback_ = ((pcf_policy_ & SWIFI_PCF_PGBK) != 0);
	bind_bool("retry_", &retry_);
	if (retry_) {
		advance_ = false;
	} else {
		advance_ = true;
	}
	bind_bool("realtime_", &realtime_);
	bind_bool("num_select_", &num_select_);
	num_scheduled_clients_ = 0;

	initRandomNumberGenerator();
}

SWiFiAgent::~SWiFiAgent()
{
	for (unsigned int i = 0; i < num_client_; i++) {
		delete client_list_.at(i);
	}
	client_list_.clear();
}

void SWiFiAgent::restart()
{
	if (is_server_) {
		for (unsigned int i = 0; i < num_client_; i++) {
			client_list_.at(i)->exp_pkt_id_ = 0;
			client_list_.at(i)->num_data_pkt_ = 0;
			client_list_.at(i)->queue_length_ = 0;
		}
		poll_state_ = SWiFi_POLL_NONE;
		if (retry_) {
			advance_ = false;
		} else {
			advance_ = true;
		}
	} else {
		num_data_pkt_ = 0;
	}
	target_ = NULL;
}

void SWiFiAgent::reset()
{
	for (unsigned int i = 0; i < num_client_; i++) {
		delete client_list_.at(i);
	}
	client_list_.clear();
	num_client_ = 0;
	target_ = NULL;

	num_data_pkt_ = 0;

	poll_state_ = SWiFi_POLL_NONE;
	if (retry_) {
		advance_ = false;
	} else {
		advance_ = true;
	}
}
// ************************************************
// Table of commands:
// 1. sw_(0) server
// 2. sw_(0) restart
// 3. sw_(0) send
// 4. sw_(0) report
// 5. sw_(0) update_delivered/update_failed
// 6. sw_(0) mac $mymac
// 7. sw_(0) register $pn $qn $init $tier
// 8. sw_(0) poll
// ************************************************

int SWiFiAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "server") == 0) {
			is_server_ = true;
			if (do_poll_num_) {
				poll_state_ = SWiFi_POLL_NUM;
			} else {
				poll_state_ = SWiFi_POLL_DATA;
			}
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "update_delivered") == 0){ //TODO: receive an ACK
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "update_failed") == 0){ //TODO:
			printf("failed!\n");
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "restart") == 0){
			restart();
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "report") == 0) { //TODO: Print data into a txt file
			tracefile_.open("report.txt");
			/*
			   Define your own report format
			   */
			tracefile_.close();
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "send") == 0) {
			if (!is_server_) {
				Tcl& tcl = Tcl::instance();
				tcl.add_error("Only server AP can send downlink data.");
				return (TCL_ERROR);
			}
			scheduleRoundRobin(true);
			// Create a new packet
			Packet* pkt = allocpkt();
			// Access the header for the new packet:
			hdr_swifi* hdr = hdr_swifi::access(pkt);
			// Let the 'ret_' field be 0, so the receiving node
			// knows that it has to generate an echo packet
			hdr->ret_ = 3; // It's a data packet
			hdr->seq_ = seq_++;
			// Store the current time in the 'send_time' field
			hdr->send_time_ = Scheduler::instance().clock();
			// Set IP addr
			hdr_ip *ip = hdr_ip::access(pkt);
			ip->saddr() = AP_IP;
			ip->daddr() = target_->addr_;
			// Broadcasting only. Need to specify ip and ACK address later on.			
			send(pkt, 0);  

			// return TCL_OK, so the calling function knows that
			// the command has been processed
			return (TCL_OK);   
		}
		else if (strcmp(argv[1], "poll") == 0) {
			if (!is_server_) {
				Tcl& tcl = Tcl::instance();
				tcl.add_error("Only server AP can poll.");
				return (TCL_ERROR);
			}
			if (poll_state_ == SWiFi_POLL_NUM) {
				if (!selective_) {
					scheduleRoundRobin(false);
				} else {
					scheduleSelectively();
				}
				if (!target_) {
					// No more clients to poll num. Poll data.
					poll_state_ = SWiFi_POLL_DATA;
				}
			}
			if (poll_state_ == SWiFi_POLL_DATA) {
				if (do_poll_num_) {
					scheduleMaxWeight();
				} else {
					target_ = client_list_.front();
				}
			}
			if (!target_) {
				if (selective_ && (num_scheduled_clients_ < num_client_)) {
					//fprintf(stderr, "Let's be work conserving!\n");
					scheduleSelectively();
					poll_state_ = SWiFi_POLL_NUM;
				} else {
					// No more clients to schedule. Idle.
					poll_state_ = SWiFi_POLL_IDLE;
					return (TCL_OK);
				}
			}
			// Create a new packet
			Packet* pkt = allocpkt();
			// Access the header for the new packet:
			hdr_swifi* hdr = hdr_swifi::access(pkt);
			// Set packet type per state
			if (poll_state_ == SWiFi_POLL_NUM) {
				if (piggyback_) {
					hdr->ret_ = SWiFi_PKT_POLL_PGBK; // It's a POLL_PGBK packet
				} else {
					hdr->ret_ = SWiFi_PKT_POLL_NUM; // It's a POLL_NUM packet
				// If no retry, send out POLL_NUM once and
				// go to POLL_DATA.
				if (!retry_ && selective_ && ((int)num_scheduled_clients_ > num_select_)) {
					poll_state_ = SWiFi_POLL_DATA;
				}
			} else if (poll_state_ == SWiFi_POLL_DATA) {
				hdr->ret_ = SWiFi_PKT_POLL_DATA; // It's a POLL_DATA packet
			} else {
				fprintf(stderr, "You have walked into the void.\n");
				exit(1);
			}
			hdr->seq_ = seq_++;
			// Store the current time in the 'send_time' field
			hdr->send_time_ = Scheduler::instance().clock();
			// Set packet size = 0
			HDR_CMN(pkt)->size() = 0;
			// Set IP addr
			hdr_ip *ip = hdr_ip::access(pkt);
			ip->saddr() = AP_IP;
			ip->daddr() = target_->addr_;
			hdr->exp_pkt_id_ = target_->exp_pkt_id_;
			//fprintf(stderr, "POLL: target addr=%d, exp_pkt_id_=%d, queue_length_=%d\n", target_->addr_, target_->exp_pkt_id_, target_->queue_length_);
			// Send it out
			send(pkt, 0);
			// return TCL_OK, so the calling function knows that
			// the command has been processed
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "boi") == 0) {
			if (is_server_) {
				target_ = NULL;
				if (do_poll_num_) {
					poll_state_ = SWiFi_POLL_NUM;
				} else {
					poll_state_ = SWiFi_POLL_DATA;
				}
				if (realtime_) {
					for (unsigned i = 0; i < num_client_; i++) {
						client_list_.at(i)->num_data_pkt_ = 0;
						client_list_.at(i)->exp_pkt_id_ = 0;
						client_list_.at(i)->queue_length_ = 0;
					}
				}
				if (selective_) {
					num_scheduled_clients_ = 0;
					// initPermutation can be done only after all
					// cliens register.
					// It is only necessary at the first interval.
					// Re-run it at each interval might have a small
					// performance loss,
					// but the good thing is that
					// we do not need yet another tcl command.
					initPermutation();
					randomPermutation(num_client_);
				}
			}
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
	        if (strcmp(argv[1], "mac") == 0){ 
			mac_ = (Mac*)TclObject::lookup(argv[2]);
			if(mac_ == 0) {
			//TODO: printing...
				printf("mac error!\n");
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "pour") == 0){
			if (atoi(argv[2]) >= 0) {
				if (realtime_) {
					num_data_pkt_ = atoi(argv[2]);
				} else {
					num_data_pkt_ += atoi(argv[2]);
				}
				//fprintf(stderr, "%s current number of data packets = %d \n", name(), num_data_pkt_);
				Tcl& tcl = Tcl::instance();
				tcl.evalf("%s alog %d", name(), num_data_pkt_);
			}
			else {
				printf("arrivals should be nonnegative!\n");
			}
			return (TCL_OK);
		}
	}
	else if (argc == 4) {
		if (strcmp(argv[1], "register") == 0) {
			if (!is_server_) {
				return (TCL_ERROR);
			}

			SWiFiClient* client = new SWiFiClient;
			client_list_.push_back(client);
			client_list_[num_client_]->addr_ = (u_int32_t)atoi(argv[2]);
			client_list_[num_client_]->pn_ = atof(argv[2]);
			client_list_[num_client_]->exp_pkt_id_ = 0;
			// Note here num_data_pkt_ only counts those received.
			client_list_[num_client_]->num_data_pkt_ = 0;
			client_list_[num_client_]->queue_length_ = 0;
			fprintf(stderr, "Registered: client %d, ip %d\n", num_client_, client_list_[num_client_]->addr_);
			num_client_++;
			return (TCL_OK);
		}
	}
	// If the command hasn't been processed by VoD_ScheduleAgent()::command,
	// call the command() function for the base class
	return (Agent::command(argc, argv));
}

void SWiFiAgent::recv(Packet* pkt, Handler*)
{
	// Access the IP header for the received packet:
	hdr_ip* hdrip = hdr_ip::access(pkt);
	//hdr_mac* hdrmac = hdr_mac::access(pkt); // Not in use for now

	// Access the header for the received packet:
	hdr_swifi* hdr = hdr_swifi::access(pkt);

	//this packet is to register  
	if (hdr->ret_ == 0){ 
		if(is_server_ == true){  //only server needs to deal with registration
			SWiFiClient* client = new SWiFiClient;
			client_list_.push_back(client);
			client_list_[num_client_]->addr_ = (u_int32_t)hdrip->saddr();
			client_list_[num_client_]->tier_ = hdr->tier_;
			client_list_[num_client_]->qn_ = hdr->qn_;
			client_list_[num_client_]->pn_ = hdr->pn_; // Now in use. 
			client_list_[num_client_]->is_active_ = true;
			client_list_[num_client_]->exp_pkt_id_ = 0;
			// Note here num_data_pkt_ only counts those received.
			client_list_[num_client_]->num_data_pkt_ = 0;
			client_list_[num_client_]->init_ = hdr->init_;
			client_list_[num_client_]->queue_length_ = hdr->init_;
			//fprintf(stderr, "Registered: client %d, ip %d\n", num_client_, client_list_[num_client_]->addr_);
			num_client_++;
		} 
		Packet::free(pkt);
		return;
	}
	// This is a data packet from client to server (UPLINK).
	// Assume it is the reply of POLL_DATA packet from server to client.
	else if (hdr->ret_ == 2) {
		if (is_server_ && pkt->userdata()
			       && pkt->userdata()->type() == PACKET_DATA) {
			//fprintf(stderr, "Received uplink data packet: src addr=%d, dest addr=%d\n", hdrip->saddr(), hdrip->daddr());
			target_->queue_length_--;
			target_->exp_pkt_id_++;
			target_->num_data_pkt_++;
			//fprintf(stderr, "addr=%d, queue_length_=%d, exp_pkt_id_=%d, num_data_pkt_=%d\n",
					//hdrip->saddr(), target_->queue_length_, target_->exp_pkt_id_, target_->num_data_pkt_);
			PacketData* data = (PacketData*)pkt->userdata();
			// Use tcl.eval to call the Tcl
			// interpreter with the poll results.
			// Note: In the Tcl code, a procedure
			// 'Agent/SWiFi recv {from rtt data}' has to be defined
			// which allows the user to react to the poll result.
			// Calculate the round trip time
			Tcl& tcl = Tcl::instance();
			tcl.evalf("%s recv %d %.3f \"%s\"", name(),
					hdrip->src_.addr_ >> Address::instance().NodeShift_[1],
					(Scheduler::instance().clock()-hdr->send_time_) * 1000,
					data->data());
		}
		Packet::free(pkt);
		return;
	}
    // A data packet from server to client was received. 
	else if (hdr->ret_ == 3){	
		if(!is_server_==true){
		// Send an user-defined ACK to server
		//first save the old packet's send_time
		double stime = hdr->send_time_;
		int rcv_seq = hdr->seq_;
		// Discard the packet
		Packet::free(pkt);
		//create a new packet
	    Packet* pkt_ACK = allocpkt();
	    //Access packet header for the new packet
		hdr_swifi* hdr_ACK = hdr_swifi::access(pkt_ACK);
		//Set the ret to 7, so the receiver won't send another echo
		hdr_ACK->ret_= 7;
		//set the send_time field to the correct value
		hdr_ACK->send_time_ = stime;
		hdr_ACK->seq_ = rcv_seq;
		// Set packet size = 0
		HDR_CMN(pkt)->size() = 0;
		//Fill in the data payload
		char *msg = "I have received the packet!";
		PacketData *data = new PacketData(1+strlen(msg));
		strcpy((char*)data->data(),msg);
		pkt_ACK->setdata(data);
		//send the packet
		send(pkt_ACK,0);
		}
		
		return;
		
		
	}

	// An user-defined ACK packet from client to server
	else if(hdr->ret_ ==7){
		if (is_server_ && pkt->userdata()
			       && pkt->userdata()->type() == PACKET_DATA) {
			PacketData* data = (PacketData*)pkt->userdata();
			// Use tcl.eval to call the Tcl
			// interpreter with the poll results.
			// Note: In the Tcl code, a procedure
			// 'Agent/SWiFi recv {from rtt data}' has to be defined
			// which allows the user to react to the poll result.
			// Calculate the round trip time
			Tcl& tcl = Tcl::instance();
			tcl.evalf("%s recv %d %.3f \"%s\"", name(),
					hdrip->src_.addr_ >> Address::instance().NodeShift_[1],
					(Scheduler::instance().clock()-hdr->send_time_) * 1000,
					data->data());
		}
		Packet::free(pkt);
		return;
	}
	else if (hdr->ret_ == SWiFi_PKT_POLL_DATA) {
		// Send a data packet from the client to the server.
		if (!is_server_) {
			//fprintf(stderr, "Received POLL_DATA: src addr=%d, dst addr=%d\n", hdrip->saddr(), hdrip->daddr());
			// Note copy assignments are necessary unless free(pkt) is delayed.
			// First save the old packet's send_time
			double stime = hdr->send_time_;
			int rcv_seq = hdr->seq_;
			u_int32_t pkt_id = hdr->exp_pkt_id_;
			nsaddr_t ret_saddr = hdrip->daddr();
			nsaddr_t ret_daddr = hdrip->saddr();
			// Discard the packet
			Packet::free(pkt);
			if (pkt_id > num_data_pkt_) {
				fprintf(stderr, "Bug: exp_pkt_id_ %d > num_data_pkt_ %d\n", pkt_id, num_data_pkt_);
				exit(1);
			}
			// Create a new packet
			Packet* pktret = allocpkt();
			// Access the packet header for the new packet:
			hdr_swifi* hdrret = hdr_swifi::access(pktret);
			// Set ret to 2 (data packet from client to server)
			hdrret->ret_ = 2;
			// Set the send_time field to the correct value
			hdrret->send_time_ = stime;
			// Added by Andrei Gurtov for one-way delay measurement.
			hdrret->rcv_time_ = Scheduler::instance().clock();
			hdrret->seq_ = rcv_seq;
			// Set destination IP addr
			hdr_ip* hdrret_ip = HDR_IP(pktret);
			hdrret_ip->saddr() = ret_saddr;
			hdrret_ip->daddr() = ret_daddr;
			// Fill in the data payload
			u_int32_t node_id = ret_saddr >> Address::instance().NodeShift_[1];
			ostringstream ss;
			ss << "Here is node " << node_id << "'s pkt no. " << pkt_id << "!";
			// Use reference to potentially extend its lifetime
			// and avoid its c_str becomes invalid.
			// cf. http://stackoverflow.com/a/1374485/1166587
			const string& str = ss.str();
			PacketData *data = new PacketData(1 + str.size());
			strcpy((char*)data->data(), str.c_str());
			pktret->setdata(data);
			// Send the packet
			send(pktret, 0);
			//fprintf(stderr, "Sent data: src addr=%d, dst addr=%d\n", hdrret_ip->saddr(), hdrret_ip->daddr());
		}
	}
	else if (hdr->ret_ == SWiFi_PKT_POLL_NUM) {
		if (!is_server_) {
			// Note copy assignments are necessary unless free(pkt) is delayed.
			double stime = hdr->send_time_;
			int rcv_seq = hdr->seq_;
			nsaddr_t ret_saddr = hdrip->daddr();
			nsaddr_t ret_daddr = hdrip->saddr();
			// Discard the packet
			Packet::free(pkt);
			// Create a new packet
			Packet* pktret = allocpkt();
			// Access the packet header for the new packet:
			hdr_swifi* hdrret = hdr_swifi::access(pktret);
			// Set ret to uplink packet with num of pkts at client
			hdrret->ret_ = SWiFi_PKT_NUM_UL;
			// Set the send_time field to the correct value
			hdrret->send_time_ = stime;
			// Added by Andrei Gurtov for one-way delay measurement.
			hdrret->rcv_time_ = Scheduler::instance().clock();
			hdrret->seq_ = rcv_seq;
			// Set packet size = 0
			HDR_CMN(pkt)->size() = 0;
			// Set destination IP addr
			hdr_ip* hdrret_ip = HDR_IP(pktret);
			hdrret_ip->saddr() = ret_saddr;
			hdrret_ip->daddr() = ret_daddr;
			hdrret->num_data_pkt_ = num_data_pkt_;
			// Send the packet
			send(pktret, 0);
			//fprintf(stderr, "Sent num %d: src addr=%d, dst addr=%d\n", num_data_pkt_, hdrret_ip->saddr(), hdrret_ip->daddr());
		}
	}
	// This is a SWiFi_PKT_POLL_PGBK packet from the server to client
	else if (hdr->ret_ == SWiFi_PKT_POLL_PGBK) {
		if (!is_server_) {
			// Note copy assignments are necessary unless free(pkt) is delayed.
			// First save the old packet's send_time
			double stime = hdr->send_time_;
			int rcv_seq = hdr->seq_;
			nsaddr_t ret_saddr = hdrip->daddr();
			nsaddr_t ret_daddr = hdrip->saddr();
			u_int32_t pkt_id = hdr->exp_pkt_id_;
			// Discard the packet
			Packet::free(pkt);
			// Create a new packet
			Packet* pktret = allocpkt();
			// Access the packet header for the new packet:
			hdr_swifi* hdrret = hdr_swifi::access(pktret);
			// Set ret to SWiFi_PKT_PGBK_UL (piggyback packet from client to server)
			hdrret->ret_ = SWiFi_PKT_PGBK_UL;
			// Set the send_time field to the correct value
			hdrret->send_time_ = stime;
			// Added by Andrei Gurtov for one-way delay measurement.
			hdrret->rcv_time_ = Scheduler::instance().clock();
			hdrret->seq_ = rcv_seq;
			// Set destination IP addr
			hdr_ip* hdrret_ip = HDR_IP(pktret);
			hdrret_ip->saddr() = ret_saddr;
			hdrret_ip->daddr() = ret_daddr;
			// Append the number of the data packets
			hdrret->num_data_pkt_ = num_data_pkt_;
			// Fill in the data payload if there is an available data packet
			if (num_data_pkt_ > 0) {
				pkt_id = 1; //FIXME: only work for real-time scenarios
				u_int32_t node_id = ret_saddr >> Address::instance().NodeShift_[1];
				ostringstream ss;
				ss << "Here is node " << node_id << "'s pkt no. " << pkt_id << "!";
				// Use reference to potentially extend its lifetime
				// and avoid its c_str becomes invalid.
				// cf. http://stackoverflow.com/a/1374485/1166587
				const string& str = ss.str();
				PacketData *data = new PacketData(1 + str.size());
				strcpy((char*)data->data(), str.c_str());
				pktret->setdata(data);
			} else {
				HDR_CMN(pkt)->size() = 0;// Set packet size = 0
			}
			// Send the packet
			send(pktret, 0);

		}
	}
	// This is a SWiFi_PKT_NUM_UL packet from client to server (UPLINK).
	else if (hdr->ret_ == SWiFi_PKT_NUM_UL) {
		if (is_server_) {
			//fprintf(stderr, "Received uplink num packet: src addr=%d, dest addr=%d, num=%d\n", hdrip->saddr(), hdrip->daddr(), hdr->num_data_pkt_);
			if (retry_) {
				advance_ = true;
				num_scheduled_clients_++;
				// If we receive the POLL_NUM reply from a
				// remaining client, serve it before POLL_NUM
				// other remaining clients.
				if (selective_ && ((int)num_scheduled_clients_ >= num_select_)) {
					poll_state_ = SWiFi_POLL_DATA;
				}
			}
			// The current queue length is equal to
			// the number of data packets generated minus
			// the number of data packets received so far.
			// For realtime traffic, num_data_pkt_ should be 0.
			if (realtime_ && target_->num_data_pkt_ != 0) {
				fprintf(stderr, "You have found a bug about num_data_pkt_!\n");
				exit(1);
			}
			target_->queue_length_ =
				hdr->num_data_pkt_ - target_->num_data_pkt_;
			//fprintf(stderr, "addr=%d, rx=%d, queue_length_=%d\n", target_->addr_, target_->num_data_pkt_, target_->queue_length_);
			if (target_->exp_pkt_id_ == 0 && target_->queue_length_ > 0) {
				target_->exp_pkt_id_ = 1;
			}
			Tcl& tcl = Tcl::instance();
			tcl.evalf("qlog %d %d", target_->addr_, target_->queue_length_);
		}
		Packet::free(pkt);
		return;
	}
	// This is a SWiFi_PKT_PGBK_UL packet from client to server (UPLINK)
	else if (hdr->ret_ == SWiFi_PKT_PGBK_UL) { //FIXME: Only work for real-time scenarios
		if (is_server_) {
			if (retry_) {
				advance_ = true;
				num_scheduled_clients_++;
				// If we receive the POLL_NUM reply from a
				// remaining client, serve it before POLL_NUM
				// other remaining clients.
				if (selective_ && ((int)num_scheduled_clients_ >= num_select_)) {
					poll_state_ = SWiFi_POLL_DATA;
				}
			}
			// The current queue length is equal to
			// the number of data packets generated minus
			// the number of data packets received so far.
			// For realtime traffic, num_data_pkt_ should be 0.
			if (realtime_ && target_->num_data_pkt_ != 0) {
				fprintf(stderr, "You have found a bug about num_data_pkt_!\n");
				exit(1);
			}
			Tcl& tcl = Tcl::instance();
			if (hdr->num_data_pkt_ > 0) {
				PacketData* data = (PacketData*)pkt->userdata();
				// Note: In the Tcl code, a procedure
				// 'Agent/SWiFi recv {from rtt data}' has to be defined
				// which allows the user to react to the poll result.
				// Calculate the round trip time
				tcl.evalf("%s recv %d %.3f \"%s\"", name(),
					hdrip->src_.addr_ >> Address::instance().NodeShift_[1],
					(Scheduler::instance().clock()-hdr->send_time_) * 1000,
					data->data());
				target_->queue_length_ =
					hdr->num_data_pkt_ - target_->num_data_pkt_ - 1;
			} else {
				target_->queue_length_ = 0;	
			}
			if (target_->exp_pkt_id_ == 0 && target_->queue_length_ > 0) {
				target_->exp_pkt_id_ = 2; // just receive the first data packet from target
			}
			tcl.evalf("qlog %d %d", target_->addr_, target_->queue_length_);
		}
		Packet::free(pkt);
		return;			
	}
}

void SWiFiAgent::scheduleRoundRobin(bool loop)
{
	if (!is_server_) {
		return;
	}
	if (!target_) {
		target_ = client_list_.at(0);
		return;
	}
	if (advance_) {
		if (!loop && target_ == client_list_.back()) {
			target_ = NULL;
			return;
		}
		for (u_int32_t i = 0; i < num_client_; i++) {
			if (target_ == client_list_.at(i)) {
				target_ = client_list_.at((i + 1) % num_client_);
				if (retry_) {
					advance_ = false;
				}
				return;
			}
		}
	}
}

void SWiFiAgent::initPermutation()
{
	client_permutation_.clear();
	for (unsigned i = 0; i < num_client_; i++) {
		client_permutation_.push_back(i);
	}
}

void SWiFiAgent::initRandomNumberGenerator()
{
	random_device rd;
	rng.seed(rd());
}

// Random k-permutation
void SWiFiAgent::randomPermutation(unsigned k)
{
	if (k > num_client_) {
		fprintf(stderr, "randomPermutation: k should be at most num_client_\n");
		exit(1);
	}
	for (unsigned i = 0; i < k; i++) {
		uniform_int_distribution<int> randint(i, num_client_ - 1);
		unsigned j = randint(rng);
		swap(client_permutation_.at(i), client_permutation_.at(j));
	}
	//fprintf(stderr, "client_permutation_: [%d", client_permutation_.front());
	//for (unsigned i = 1; i < num_client_; i++) {
		//fprintf(stderr, ", %d", client_permutation_.at(i));
	//}
	//fprintf(stderr, "]\n");
}

// Once randomPermutation is done, the first num_select_
// elements of client_permutation_ is the selective schedule.
void SWiFiAgent::scheduleSelectively()
{
	if (!is_server_) {
		return;
	}
	if (num_scheduled_clients_ >= num_client_) {
		target_ = NULL;
	} else {
		unsigned idx = client_permutation_.at(num_scheduled_clients_);
		target_ = client_list_.at(idx);
		if (!retry_) {
			num_scheduled_clients_++;
		}
	}
}

// Schedule uplink packet transmission with the Max Weight policy
void SWiFiAgent::scheduleMaxWeight()
{
	if (!is_server_ || num_client_ < 1) {
		target_ = NULL;
		return;
	}

	double Wmax; // the max weight (queue length * channel reliability)

	// Start with the first client.
	// Note vector::at() is safer than operator [] due to bound checking.
	target_ = client_list_.at(0);
	Wmax = target_->queue_length_ * target_->pn_;
	for (unsigned int j = 1; j < num_client_; j++){
		if (client_list_.at(j)->queue_length_ * client_list_.at(j)->pn_ > Wmax) {
			target_ = client_list_.at(j);
			Wmax = target_->queue_length_ * target_->pn_;
		}
	}
	if (Wmax < TOL) {
		target_ = NULL;
	}
}
