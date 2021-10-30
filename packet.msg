//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

//
// TODO generated message class
//

cplusplus {{
	#include <vector>
	#include <deque>
	typedef std::vector<int> intvec;
	typedef std::vector<void*> ptrvec;
	typedef std::deque<int> intdeque;
}}

// if want to use this starting from omnet6.0, you should include a makefrag with
// folloowing lines:
// 
// MSGC:=$(MSGC) --msg4
//   
class noncobject intvec;
class noncobject ptrvec;
class noncobject intdeque;

// otherwise use the latest notation:

//class vec {
//	@existingClass;
//}

struct Flow {
	
	string id;	// flow id
    long size;	// flow size
    int seq = 0;// transmitted packets
    int src;	// source
	int dst; // destination	
	int dstIndex;
	long app;		// random [1, 2**32] representing app
//	int hopCount = 0;	// at flow level 
	
	ptrvec SRI;		// Source Routing Intformation (i.e., list of gates)
	
	// Indicator where a flow has been recorded. 
	// Each switch adds pointer to itself in case FWI = 1 (i.e., flow has been monitored)
	ptrvec useSketch;
	
	// boolean array Fragment Write Information: 1 if fragment has to be used
	// Note: the size of this depends on the path length and each hop will change its size by removing 
	// its own FWI
	intdeque FWI;
}

message Packet {
    //int seq = 0;
    Flow flow;
    int route[];	// source routing: here interface ids where to send
    long min = -1;	// current minimum for CMS
    
}
