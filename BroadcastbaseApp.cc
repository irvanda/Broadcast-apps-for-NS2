#include "timer-handler.h"
#include "packet.h"
#include "app.h"
#include "address.h"
#include "ip.h"
#include "bbcast-packet.h"
#include "BroadcastbaseAgent.h"
#include "node.h"
#include "BroadcastbaseApp.h"
#include "random.h"

#include <math.h>
#include <mobilenode.h>
#include <stdlib.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
/* ----- used for debug printing...just comment them or no!! ------ */
#define DEBUG_BROADCAST 

extern FILE * nhopFile ;
extern FILE * traceFile ;

/* broadcast type
	0 = area-based
	1 = distance-based
	2 = rss-based
	3 = conv-min
	4 = conv-max
*/

/* BroadcastbaseApp OTcl linkage class */
static class BroadcastbaseAppClass : public TclClass {
public:
	BroadcastbaseAppClass() : TclClass("Application/BroadcastbaseApp") {}
	TclObject* create(int, const char*const*) 
	{
		return (new BroadcastbaseApp);
	}
} class_app_broadcastbase;

/* -- this is the expiration timer to manage the interval of cbr --*/
void MyBCastCbrTimer::expire(Event*)
{
	if (t_->running_) t_->cbr_broadcast();
}

/* -- this is the expiration timer called when randow cw timer expires --*/
void MyBCastCWTimer::expire(Event* e)
{	
	t_->rcv_pkt[p_].nhop=hops_+1;
	t_->rcv_pkt[p_].slot_waiten=slot_waiten_;
	
	if (!(t_->rcv_pkt[p_].value) ) { /* I haven't still received pkt from opposite direction... I have to send it! */
				
		//print: forward time src_addr pkt_id=flow_id
		fprintf (traceFile,"f %.9f %d %d\n",Scheduler::instance().clock(),nodeid,p_);
		fflush(traceFile);
		fflush(stdout);
		t_->rcv_pkt[p_].value=true; /* I'm sending p_, so I won't send it again!! */
		t_->send_broadcast(p_);
		free(t_->rcv_pkt[p_].cwt); /* I don't need this timer any more time */
		
	}
	/* else I've already received this packet form opposite direction...I don't need to forward it! Do nothing! */
}

/* Constructor (also initialize instances of timers) */
BroadcastbaseApp::BroadcastbaseApp() : cbr_timer_(this) , cw_timer_(this,1,1,1,1)
{	running_=1;
	bind("bmsg-interval_", &binterval); // interval for broadcast msg
	bind("bsize_", &bsize); //size of broadcast msg
	bind("radius_", &rad); //radius
	bind("Distance_", &D); //distance to P(x,y)
	bind("bcast-type_", &bcastType); // broadcast type
	bind("MaxRange_", &distMax); // maximum distance for distance-based
	bind("RSSMin_", &rssMin); // minimum rss for rss-based
	bind("RSSMax_", &rssMax); // maximum rss for rss-based
	bind("DestNode_", &destNode); // destination node
	binterval=0;
	rnd_time=0;
}

/* -- OTcl command interpreter --*/
int BroadcastbaseApp::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) 
	{ 
		if (strcmp(argv[1], "attach-agent") == 0) 
		{
			agent_ = (Agent*) TclObject::lookup(argv[2]);
			if (agent_ == 0) 
			{
				tcl.resultf("no such agent %s", argv[2]);
				return(TCL_ERROR);
			}
			agent_->attachApp(this);
			return(TCL_OK);
		} 
	}
	if (argc == 2) 
	{ 
		if (strcmp(argv[1], "send-broadcast") == 0) 
		{
			send_broadcast((long int)-1);		
			return(TCL_OK);
		} 
		if (strcmp(argv[1], "cbr-broadcast") == 0) 
		{
			cbr_broadcast();		
			return(TCL_OK);
		} 
 		if (strcmp(argv[1], "print-trace") == 0) 
 		{
 			print_trace();		
 			return(TCL_OK);
		} 
	}
	return (Application::command(argc, argv));
}



/* --simulation of a cbr using broadcast pkt --*/
void BroadcastbaseApp::cbr_broadcast() {
	send_broadcast((long int)-1);
	if(binterval > 0)  cbr_timer_.resched(binterval);
}

/* --sending of broadcast pkt --*/
// void BroadcastbaseApp::send_broadcast(long int id, char direction)
void BroadcastbaseApp::send_broadcast(long int id)
{
  	double x,y,z;
	hdr_bbcast mh_buf;
	nsaddr_t t;
	t=((BroadcastbaseAgent*)agent_)->addr();
	MobileNode *test = (MobileNode *) (Node::get_node_by_address(t));

	/* setting of field in the pkt. I'm preparing to send! */
	test->getLoc(&x,&y,&z);	
	mh_buf.x_=x;
	mh_buf.y_=y;
	
	BBCastData *data = new BBCastData();	
	
	if (id >=0) { //forward
		mh_buf.nhop_=rcv_pkt[id].nhop; 
		mh_buf.slot_waiten_=rcv_pkt[id].slot_waiten+rcv_pkt[id].slot_choosen;
	}
	else { //send
		mh_buf.nhop_=0;
		mh_buf.slot_waiten_=0;
	}
	data->setHeader(&mh_buf);
	((BroadcastbaseAgent*)agent_)->sendbroadcastmsg(bsize, data, id);
	#ifdef DEBUG_BROADCAST
	cout << "BroadcastMsg - SENT \t- Node "<<((BroadcastbaseAgent*)agent_)->addr()<< "  pkt "<<id<<"\n";
	fflush(stdout);	
	#endif
}

void BroadcastbaseApp::start()
{	
	cbr_broadcast();	
}

void BroadcastbaseApp::stop() {
 running_=0;		
}

		
void BroadcastbaseApp::print_trace()
{
   
   map <long int,data_packet>::iterator it;
   
   	if (((BroadcastbaseAgent*)agent_)->addr() == destNode) {
   			
	   for( it =  rcv_pkt.begin(); it !=  rcv_pkt.end(); it++ ) {
			
		   //print: node pkt hops slots
		   fprintf(nhopFile,"n %d %d %d %d\n",((BroadcastbaseAgent*)agent_)->addr(),(*it).first,rcv_pkt[(*it).first].nhop,rcv_pkt[(*it).first].slot_waiten); 
		   fflush(nhopFile);
		 
	   }
   
	}
   	
}

/* Schedule next data packet transmission time */
double BroadcastbaseApp::next_snd_time()
{    	
	return interval;
}

/* Receive message from underlying agent */
void BroadcastbaseApp::process_data_BroadcastMsg(char *pkt, hdr_ip* ih, double rss)
{
	BBCastData *data = new BBCastData(pkt);
	//looking for my position
	nsaddr_t t;
	t=((BroadcastbaseAgent*)agent_)->addr();
	MobileNode *test = (MobileNode *) (Node::get_node_by_address(t));
	double mpx,mpy,mpz;
	test->getLoc(&mpx,&mpy,&mpz);
	
	//sender position
	double spx=data->g_x();
	double spy=data->g_y();
	
	if (bcastType == 0){
		//P(x,y) as the center of relay selection area
		double px=spx+D;
		double py=spy;
		//receiver distance from P(x,y)
		double dist=sqrt(pow((mpx - px), 2) + pow((mpy - py), 2));
		//call the relay selection function
		bool relay_ = select_relay(dist);
		
		if(relay_){ /* I'm relay node */
			
			#ifdef DEBUG_BROADCAST
			cout<<"BroadcastMsg - Node "<<((BroadcastbaseAgent*)agent_)->addr() <<" is a relay node "<< endl;
			#endif
			
			if(rcv_pkt.find(data->g_id())==rcv_pkt.end()){
				broadcast_procedure(data->g_id(),dist,NULL,data->g_nhop(),data->g_slot_waiten());
			}
			else {
				rcv_pkt[data->g_id()].value=true;
			}
		}
		else{
			#ifdef DEBUG_BROADCAST
			cout<<"BroadcastMsg - Node "<<((BroadcastbaseAgent*)agent_)->addr() <<" is not a relay node "<< endl;
			#endif
		}
		fprintf (traceFile,"r %.9f %d %d\n",Scheduler::instance().clock(),((BroadcastbaseAgent*)agent_)->addr(),data->g_id());
		fflush(traceFile);
	}
	else if(bcastType == 1){ //dist-based
		//sender distance from me
		double d=sqrt(pow((mpx - spx), 2) + pow((mpy - spy), 2));
		if (rcv_pkt.find(data->g_id())==rcv_pkt.end()){
			broadcast_procedure(data->g_id(),d,NULL,data->g_nhop(),data->g_slot_waiten());
		}
		else {
			rcv_pkt[data->g_id()].value=true;
		}
		fprintf (traceFile,"r %.9f %d %d\n",Scheduler::instance().clock(),((BroadcastbaseAgent*)agent_)->addr(),data->g_id());
		fflush(traceFile);
	}
	else if(bcastType == 2) { // rss-based
		if (rcv_pkt.find(data->g_id())==rcv_pkt.end()){
			broadcast_procedure(data->g_id(),NULL,rss,data->g_nhop(),data->g_slot_waiten());
		}
		else {
			rcv_pkt[data->g_id()].value=true;
		}
		fprintf (traceFile,"r %.9f %d %d\n",Scheduler::instance().clock(),((BroadcastbaseAgent*)agent_)->addr(),data->g_id());
		fflush(traceFile);	
	}
	else{
		if(rcv_pkt.find(data->g_id())==rcv_pkt.end()) { 
			// //print: received time node pkt
			broadcast_procedure(data->g_id(),NULL,NULL,data->g_nhop(),data->g_slot_waiten());
		}
		else {  //received a msg already heard in the past		
			 return;
		}
		fprintf (traceFile,"r %.9f %d %d\n",Scheduler::instance().clock(),((BroadcastbaseAgent*)agent_)->addr(),data->g_id());
		fflush(traceFile);
	}
}

// Relay selection
bool BroadcastbaseApp::select_relay(double dist){
bool relay;
if (dist < rad)
		relay=true; /* I'm relay node */
else
		relay=false; /* I'm not relay node */
return relay;
}

/* choose a value into CW and wait */
void BroadcastbaseApp::broadcast_procedure(long int gid, double dist, double rss, int hops, long int slot_w){
	int cw, rslot; //used for value of contention window
	double r;
	if (bcastType == 0) { //area-based
		//I get contention window (in slot)
		cw = CWMin; //for conventional broadcast
		//random slot from CW
		Random::seed_heuristically();	 		 
		rslot=Random::integer(cw-1);
		r=rslot * SLOT;
		#ifdef DEBUG_BROADCAST
		cout<<"BroadcastMsg - Node "<<((BroadcastbaseAgent*)agent_)->addr() <<" value choosen "<<rslot<<" inside CW which is "<<r<<" sec\n";
		#endif
	}
	else if(bcastType == 1){ //distance-based
		//I get contention window (in slot)
		cw = compute_contwnd_dist(dist,distMax);	 
		//random slot from CW
		Random::seed_heuristically();	 		 
		rslot=Random::integer(cw-1);
		r=rslot* SLOT;
		#ifdef DEBUG_BROADCAST
		cout<<"BroadcastMsg - Node "<<((BroadcastbaseAgent*)agent_)->addr() <<" value choosen "<<rslot<<" inside CW which is "<<r<<" sec\n";
		#endif
	}
	else if(bcastType == 2){ // rss based
		//I get contention window (in slot)
		cw = compute_contwnd_rss(rss, rssMin, rssMax); //for rss broadcast
		//random slot from CW
		Random::seed_heuristically();	 		 
		rslot=Random::integer(cw-1);
		r=rslot * SLOT;
		#ifdef DEBUG_BROADCAST
		cout<<"BroadcastMsg - Node "<<((BroadcastbaseAgent*)agent_)->addr() <<" value choosen "<<rslot<<" inside CW which is "<<r<<" sec\n";
		#endif
	}
	else{
		if(bcastType == 3)
			cw = CWMin; //for conventional broadcast
		else
			cw = CWMax; //for conventional broadcast
		//random slot from CW
		Random::seed_heuristically();	 		 
		//int rslot=cw;
		rslot=Random::integer(cw-1);
		r=rslot * SLOT;
		#ifdef DEBUG_BROADCAST
		cout<<"BroadcastMsg - Node "<<((BroadcastbaseAgent*)agent_)->addr() <<" value choosen "<<rslot<<" inside CW which is "<<r<<" sec\n";
		#endif
	}
	
	MyBCastCWTimer *cwt =new MyBCastCWTimer(this,gid,((BroadcastbaseAgent*)agent_)->addr(),hops,slot_w);
	/* create a new data packet structure with value=false before waiting time */
	data_packet dp;
	dp.value=false;
	dp.slot_choosen=rslot;
	dp.slot_waiten=0;
	dp.nhop=0;		
	dp.cwt=cwt;
	
	/* Insert of a new pkt in the map with value=false (i will send it if don't change ) and his timer */
	pair<long int,data_packet> Enumerator(gid,dp);
	rcv_pkt.insert(Enumerator);
	cwt->resched(r); /* new timer with the random value computed by cw */
}

int BroadcastbaseApp::compute_contwnd_rss(double rss, double rssMin, double rssMax){
	int tmp;
	if(rss>rssMin)
		tmp=(int)((((rss-rssMin)/(rssMax-rssMin))*(CWMax-CWMin))+CWMin);
	else
		tmp=(int)((((rssMin-rss)/(rssMax-rssMin))*(CWMax-CWMin))+CWMin);
	
	return tmp;
}

int BroadcastbaseApp::compute_contwnd_dist(double dist, double distMax){
	int tmp;
	if(dist<distMax)
		tmp=(int)((((distMax-dist)/distMax)*(CWMax-CWMin))+CWMin);
	else
		tmp=(int)((((dist-distMax)/distMax)*(CWMax-CWMin))+CWMin);
	return tmp;
}

