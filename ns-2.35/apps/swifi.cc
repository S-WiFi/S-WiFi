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
	qn_ = 0;	    //throughput requirement
	tier_ = 1;	    //tier of this client
	pn_ = 0;        //channel reliability
}

SWiFiAgent::SWiFiAgent() : Agent(PT_SWiFi), seq_(0), mac_(0)
{ //TODO: Revise...
	num_client_ = 0;
	client_list_ = vector<SWiFiClient*>();
	is_server_ = 0;
	target_ = 0;
	packet_size_ = 0;
	bind("packet_size_", &packet_size_);
	bind("slot_interval_", &slot_interval_);
}

SWiFiAgent::~SWiFiAgent()
{ //TODO: Revise...
	for (unsigned int i = 0; i < client_list_.size(); i++) {
		delete client_list_[i];
	}
	client_list_.clear();
}

void SWiFiAgent::Reset()
{
	// Reset client list.
	for (unsigned int i = 0; i < client_list_.size(); i++) {
		delete client_list_[i];
	}
	client_list_.clear();
	num_client_ = 0;
}
// ************************************************
// Table of commands:
// 1. sw_(0) server
// 2. sw_(0) restart
// 3. sw_(0) send
// 4. sw_(0) report
// 5. sw_(0) update_delivered/update_failed
// 6. sw_(0) mac $mymac
// 7. sw_(0) register $qn $init $tier
// 8. sw_(0) poll
// ************************************************

int SWiFiAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "server") == 0) {
			is_server_ = true;
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "update_delivered") == 0){ //TODO: receive an ACK
			return (TCL_OK);
		}
		else if (strcmp(argv[1], "update_failed") == 0){ //TODO: 
			printf("failed!\n");
			return (TCL_OK);
		}
		else if (strcmp (argv[1], "restart") == 0){ //TODO: restart the simulation
			Reset();
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
			// IP information  			
			hdr_ip *ip = hdr_ip::access(pkt);
			// Set packet size
			size_ = packet_size_;
			hdr->pkt_size_ = packet_size_;
			// Broadcasting only. Need to specify ip and ACK address later on.			
			send(pkt, 0);  
			
			
			

			// return TCL_OK, so the calling function knows that
			// the command has been processed
			return (TCL_OK);   
		}
		else if (strcmp(argv[1], "poll") == 0) {
			// Create a new POLL packet
			Packet* pkt = allocpkt();
			// Access the header for the new packet:
			hdr_swifi* hdr = hdr_swifi::access(pkt);
			// Let the 'ret_' field be SWiFi_PKT_POLL, so the
			// receiving node (client)
			// knows that it has to generate an data packet
			// per Problem 2.
			hdr->ret_ = SWiFi_PKT_POLL; // It's a POLL packet
			hdr->seq_ = seq_++;
			// Store the current time in the 'send_time' field
			hdr->send_time_ = Scheduler::instance().clock();
			// Set packet size = 0
			size_ = 0;
			hdr->pkt_size_ = 0;
			// Broadcasting only. Need to specify ip address later on.			
			send(pkt, 0);
			// return TCL_OK, so the calling function knows that
			// the command has been processed
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
	}
	else if (argc == 5) {
		if (strcmp(argv[1], "register") == 0) {
			//TODO: Broadcasting a packet for link registration
			Packet* pkt = allocpkt();
			hdr_swifi* hdr = hdr_swifi::access(pkt);     
			hdr->ret_ = 0;		// a packet to register on the server
			hdr->qn_ = atoi(argv[2]);
			hdr->tier_ = atoi(argv[3]);
			hdr->init_ = atoi(argv[4]);
			send(pkt, 0);     
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
			//TODO: 
			SWiFiClient* client = new SWiFiClient;
			client_list_.push_back(client);
			client_list_[num_client_]->addr_ = (u_int32_t)hdrip->saddr();
			client_list_[num_client_]->tier_ = hdr->tier_;
			client_list_[num_client_]->qn_ = hdr->qn_;
			client_list_[num_client_]->pn_ = 0; // Not in use for now. 
			client_list_[num_client_]->is_active_ = true;
			num_client_++;
		} 
		Packet::free(pkt);
		return;
	}
	// This is a data packet from client to server (UPLINK).
	// Assume it is the reply of POLL packet from server to client.
	else if (hdr->ret_ == 2) {
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
			tcl.evalf("%s recv %d %3.1f \"%s\"", name(),
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
			tcl.evalf("%s recv %d %3.1f \"%s\"", name(),
					hdrip->src_.addr_ >> Address::instance().NodeShift_[1],
					(Scheduler::instance().clock()-hdr->send_time_) * 1000,
					data->data());
		}
		Packet::free(pkt);
		return;
			
	}
/*
	else if (hdr->ret_ == 7) {
		if(is_server_ == true) {
      		int i;
      		for(i = 0; i < numclient_; i++) {
        		if(client_list_[i]->addr_ == Ackaddr_)
          			break;
			}
    		if(i < numclient_){  
			//TODO: If transmission succeeded, set done_ to TRUE
  				client_list_[i]->done_ = true;
   			}
		}
     	return;
	}
*/
	else if (hdr->ret_ == SWiFi_PKT_POLL) {
		// Send a data packet from the client to the server.
		if (!is_server_) {
			// First save the old packet's send_time
			double stime = hdr->send_time_;
			int rcv_seq = hdr->seq_;
			// Discard the packet
			Packet::free(pkt);
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
			// Set packet size
			size_ = packet_size_;
			hdrret->pkt_size_ = packet_size_;
			// Fill in the data payload
			char *msg = "I'm feeling great!";
			PacketData *data = new PacketData(1 + strlen(msg));
			strcpy((char*)data->data(), msg);
			pktret->setdata(data);
			// Send the packet
			send(pktret, 0);
		}
	}
}
