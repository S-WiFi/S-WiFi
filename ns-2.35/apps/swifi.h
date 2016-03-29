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
#include <sstream>
#include <string>
#include <algorithm> //for min
using std::vector;
using std::endl;
using std::ofstream;
using std::ostringstream;
using std::string;
using std::min;

#define AP_IP 0 // It has to be 0 for ARP to find the AP correctly.

enum swifi_pkt_t {
	SWiFi_PKT_POLL_NUM  = 9,  // poll number of packets in uplink
	SWiFi_PKT_POLL_DATA = 10, // poll data packet transmission in uplink
	SWiFi_PKT_NUM_UL,  // packet in uplink that carrys num of pkts at client
	SWiFi_PKT_DATA_UL, // data packet in uplink (client to server)
};

enum swifi_poll_state {
	SWiFi_POLL_NONE,
	SWiFi_POLL_NUM,
	SWiFi_POLL_DATA,
	SWiFi_POLL_IDLE,
};

#define	SWiFi_PCF_BASELINE 0
#define	SWiFi_PCF_SMART    1

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

 	//For register packet
	double pn_;   // channel reliability
 	double qn_;   // user demand in packets
	int init_;    // initial data in packets in the wireless node
	int tier_;    // user priority, not in use for now

	// For polling num packet
	u_int32_t num_data_pkt_;
	// For polling data packet
	u_int32_t exp_pkt_id_;

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
	double pn_;   // channel reliability
	double qn_;   // user demand in packets
	int init_;    // initial data in packets in the wireless node
	int tier_;    // user priority, not in use for now
	bool is_active_; // indicate whether the client is active or not
	u_int32_t exp_pkt_id_;   // Expected packet index.
	u_int32_t num_data_pkt_; // Number of data packets received of client
	u_int32_t queue_length_; // Remaining queue length of client
};

class SWiFiAgent : public Agent {

public:
	SWiFiAgent();
	~SWiFiAgent();

 	int seq_;	       // a send sequence number like in real ping
	u_int32_t num_client_;  // number of total clients
	vector<SWiFiClient*> client_list_;  // For a server to handle scheduling among clients
	bool is_server_;   // Indicate whether the agent is a server or not

	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);

	void restart(); // For server to reset counters of registered clients
	void reset();   // For server to reset (un-register) associated clients

protected:
	u_int32_t Ackaddr_;    // IP address for incoming ACK packet
	double slot_interval_; // time length of a single slot
	Mac* mac_;             // MAC
	SWiFiClient* target_;  // Only for server: showing the current target client 
	ofstream tracefile_;   // For outputting user-defined trace file

	// Number of data packets generated at the client.
	// Under PCF, it is only meaningful for clients.
	// The server AP tracks this info per client in client_list_.
	// Note that for realtime traffic, it is reset per interval.
	// For non-realtime traffic, it is accumulated all the time in each run.
	u_int32_t num_data_pkt_;

	// Used by server only
	swifi_poll_state poll_state_; // Indicate the state of polling (used by server AP)
	bool advance_;  // Whether to advance to the next client in scheduling

	// Scheduling parameters
	int do_poll_num_;      // Whether to send POLL_NUM before POLL_DATA
	int pcf_policy_; // PCF policy: baseline/smart
	// Whether to retry the same client if no response (user configurable)
	int retry_;
	int realtime_;  // Whether the traffic is realtime

	void scheduleRoundRobin(bool loop); // Poll each registered client one by one
	// Schedule uplink data packet transmission with Max Weight policy
	void scheduleMaxWeight();
};

#endif //  NS_SWIFI_H
