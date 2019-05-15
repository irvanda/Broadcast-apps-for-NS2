#include "ip.h"
#include "bbcast-packet.h"

int hdr_bbcast::offset_; 

// BBCast Header Class 
static class BBCastHeaderClass : public PacketHeaderClass {
public:
	BBCastHeaderClass() : PacketHeaderClass("PacketHeader/BBCast",
						    sizeof(hdr_bbcast)) {
		bind_offset(&hdr_bbcast::offset_);
	}
} class_hdr_bbcast;

//constructor
BBCastData::BBCastData(char* buf) : AppData(BBCAST_DATA)
{
	
	x_ = ((hdr_bbcast*)buf)->x_;
	y_ = ((hdr_bbcast*)buf)->y_;
	id_ = ((hdr_bbcast*)buf)->id_;
	nhop_ = ((hdr_bbcast*)buf)->nhop_;
	slot_waiten_ = ((hdr_bbcast*)buf)->slot_waiten_;
	snode_ = ((hdr_bbcast*)buf)->snode_;
	
}

//set fields for new packet
void BBCastData::pack(char* buf) const
{		
	((hdr_bbcast*)buf)->x_ = x_;
	((hdr_bbcast*)buf)->y_ = y_;
	((hdr_bbcast*)buf)->id_=id_;
	((hdr_bbcast*)buf)->nhop_=nhop_;
	((hdr_bbcast*)buf)->slot_waiten_=slot_waiten_;
	((hdr_bbcast*)buf)->snode_ = snode_;
	
}
//set header fields
void BBCastData::setHeader(hdr_bbcast *ih)
{
	
	x_=ih->x_;
	y_=ih->y_;
	id_=ih->id_;
	nhop_=ih->nhop_;
	slot_waiten_=ih->slot_waiten_;
	snode_=ih->snode_;
}
//print packet information
void BBCastData::print()
{	
	printf("X_	:\t%f\n",x_);
	printf("Y_	:\t%f\n",y_);
	printf("ID_	:\t%f\n",id_);
	printf("HOPS_	:\t%d\n",nhop_);
	printf("SLOT WAITEN_	:\t%d\n",slot_waiten_);
	printf("SNODE_	:\t%d\n",snode_);
	fflush(stdout);
}
