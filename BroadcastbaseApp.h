#ifndef ns_bbcast_app_h
#define ns_bbcast_app_h
#include <map>

/*----------CHANGEABLE PARAMETERS----------*/
#define SLOT 0.000009
//#define SLOT 0.0006
//#define SLOT 0.0002
//#define SLOT 0.001
#define CWMin 16
#define CWMax 1024


/*-----------------------------------------*/

using namespace std;
typedef std::map<long int, bool> Fmap;

class BroadcastbaseApp;

/* Sender uses this timer to schedule next app data packet transmission time  */
class MyBCastCbrTimer : public TimerHandler {
	public:
        	MyBCastCbrTimer(BroadcastbaseApp* t) : TimerHandler(), t_(t) {}
        	virtual void expire(Event*);
 	protected:
        	BroadcastbaseApp* t_;
};

/* This timer is used to wait for the contention window choosed randomly */
class MyBCastCWTimer : public TimerHandler {
 	public:
		MyBCastCWTimer(BroadcastbaseApp* t,long int pkt,int nid, int hops, long int slot_w) : TimerHandler(), t_(t){
			p_=pkt; nodeid=nid; 
			hops_=hops; 
			slot_waiten_=slot_w;
		}
         	virtual void expire(Event*);
	protected:
        	BroadcastbaseApp* t_;
 	private:
		long int p_;	/* packet number */
		int nodeid;	/* id for node who is waitng */
		int hops_;	/* hops experienced by the packet until now */
		long int slot_waiten_;	/* slots waiten by the packet until now */
};

/* Used to store data needed to process the packet by the node */
typedef struct data_packet {
	bool value; /* true if pkt comes from behind and it musn't be forwarwed */
	MyBCastCWTimer* cwt; /* contention window timer */
	int nhop;
	long int slot_waiten; 	/* slot waiten before receive pkt (read from pkt) */
	long int slot_choosen; 	/* slot to wait before send pkt (choosed by this node) */
}data_packet;

/* Broadcastbase Application Class Definition */
class BroadcastbaseApp : public Application {
	friend class MyBCastCbrTimer;
	friend class MyBCastCWTimer;

	public:	
		//constructor
		BroadcastbaseApp(); 
		/* send a broadcast packet */
		void send_broadcast(long int id); 
		/* send packets with interval declared by user */
		void cbr_broadcast(); 
		/* select relay */
		bool select_relay(double d);
		/* cancel broadcast */
		void cancel_broadcast(long int gid);
		/* process packet received from agent */	 
		void process_data_BroadcastMsg(char *pkt, hdr_ip* ih, double rss);
		int running_; //used to stop cbr_broadcast
		int compute_contwnd_rss(double rss, double rssMin, double rssMax);
		int compute_contwnd_dist(double dist, double distMax);
	protected:
		/* OTcl command intepreter */
		int command(int argc, const char*const* argv); 
		 /* Start sending data packets (Sender) */
		void start();
		/* Stop sending data packets (Sender) */
		void stop();  
		/* print trace information */
		void print_trace(); 
	private:
		/* compute next time to schedule timer */
		inline double next_snd_time(); 
		void broadcast_procedure(long int gid,double dist, double rss, int hops,long int slot_w);
		
		MyBCastCWTimer cw_timer_;   // Contention window Timer
		MyBCastCbrTimer cbr_timer_; //Cbr Broadcast Timer
		

		double interval; 
		double binterval; //broadcast interval declared by user
		double D; //distance from sender to P(x,y)
		double rad; //radius
		double rnd_time;
		int bcastType;
		double distMax;
		double rssMin;
		double rssMax;
		int destNode;
		int bsize; // broadcast packet size declared by user
		
		/* store the pkt received information, first field is the pkt id */
		map <long int,data_packet> rcv_pkt;  
				
};
#endif
