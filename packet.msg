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
	typedef std::vector<int> vec;
}}

// if want to use this starting from omnet6.0, you should include a makefrag with
// folloowing lines:
// 
// MSGC:=$(MSGC) --msg4
//   
class noncobject vec;

// otherwise use the latest notation:

//class vec {
//	@existingClass;
//}

struct Flow {
	
	string id;	// flow id
    long size;	// flow size
    int seq = 0;// transmitted packets
    string src;	// source
	string dst; // destination	
	vec useSketch;
}

message Packet {
    //int seq = 0;
    Flow flow;
    int route[];	// source routing: here interface ids where to send
    long min = -1;	// current minimum for CMS
    
    //double arrayField1[];
    //double arrayField2[10];
}
