/*
 * swifi.cc
 * Author: Ping-Chun Hsieh (2016/02/17)
 */
// File: Code for downlink and uplink transmissions using WiFi
//

#include "swifi.h"
#include <cstdlib>

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
	bind("packet_size_", &packet_size_);
	bind("slot_interval_", &slot_interval_);
}

SWiFiAgent::~SWiFiAgent()
{ //TODO: Revise...
	for (unsigned int i = 0; i < client_list_.size(); i++) {
		delete client_list_[i];
	}
}

void SWiFiAgent::Reset()
{ //TODO: Reset all parameters and history

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

    // A data packet from server to client was received. 
	else if (hdr->ret_ == 3){	
		// Send an user-defined ACK to server
/*		Packet* pkt_ACK = allocpkt();
		hdr_swifi* hdr_ACK = hdr_swifi::access(pkt_ACK);
		hdr_ip* hdrip_ACK = hdr_ip::access(pkt_ACK);
		hdrip_ACK->daddr() = (u_int32_t)hdrip->saddr();
		hdr_ACK->ret_ = 7;
		send(pkt_ACK, 0); 
*/
		// Discard the packet
		Packet::free(pkt);
		return;
	}

	// An user-defined ACK packet from client to server
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
}

