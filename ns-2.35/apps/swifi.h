/*
 * swifi.h
 * Author: Ping-Chun Hsieh (2016/02/17)
 */
// File: Code for downlink & uplink transmissions using WiFi

// $Header: /cvsroot/nsnam/ns-2/apps/ping.h,v 1.5 2005/08/25 18:58:01 johnh Exp $


#ifndef NS_SWIFI_H
#define NS_SWIFI_H

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include "mac.h"
#include <vector>   
#include <fstream>  // For outputting data
using std::vector;
using std::endl;
using std::ofstream;

enum swifi_pkt_t {
	SWiFi_PKT_POLL = 10,
};


struct hdr_swifi {
	char ret_;
  	//ret_ indicates the packet type
  	//0 for register, 1 for request, 2 for data from client to server,
  	//3 for data from server to client, 4 for register duplex traffic
  	//5 for 802.11 traffic, client to server, 6 for register downlink traffic
	//7 for user-defined ACK from client to server

    char successful_;  	// 1 for successful transmission
	double send_time_;  // when a packet sent from the transmitter
 	double rcv_time_;	// when a packet arrived to receiver
 	int seq_;		    // sequence number
	int pkt_size_;      // size of a packet

 	//For register packet
 	double qn_;   // user demand in packets
	int tier_;    // user priority, not in use for now
	int init_;    // initial data in packets in the wireless node
	double pn_;   // channel reliability, not in use for now	

	// Header access methods
	static int offset_; 	// required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_swifi* access(const Packet* p) {
		return (hdr_swifi*) p->access(offset_);
	}
};

//primitive data for clients
class SWiFiClient {

public:
	SWiFiClient();
	bool is_active() {return is_active_;}

	u_int32_t addr_; // address
	double qn_;	     // throughput requirement
	int tier_;	     // tier of this client
	double pn_;      // channel reliability
	bool is_active_; // indicate whether the client is active or not
};

class SWiFiAgent : public Agent {

public:
	SWiFiAgent();
	~SWiFiAgent();

 	int seq_;	       // a send sequence number like in real ping
	int num_client_;  // number of total clients
	vector<SWiFiClient*> client_list_;  // For a server to handle scheduling among clients
	bool is_server_;   // Indicate whether the agent is a server or not

	virtual int command(int argc, const char*const* argv); //TODO: blablabla...
	virtual void recv(Packet*, Handler*); //TODO: blablabla...

	void SetPacketSize(int s){ packet_size_ = s;}
	void Reset(); //TODO: For server to reset parameters

protected:
	u_int32_t Ackaddr_;    // IP address for incoming ACK packet
	double slot_interval_; // time length of a single slot
	unsigned int packet_size_; // size of payload
	Mac* mac_;             // MAC
	SWiFiClient* target_;  // Only for server: showing the current target client 
	ofstream tracefile_;   // For outputting user-defined trace file
	unsigned int n_run_;   // count of simulation runs

};

#endif //  NS_SWIFI_H

