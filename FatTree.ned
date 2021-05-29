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

// Created: 2021-4-19 for project disaggregated-sketches

package sketches;

network FatTree
{
     parameters:
        int k = default(4);	// k of Fat-Tree
        int K = default(2);
        
        int numSwitches_L1 = int((k^2)/2);
        int numSwitches_L2 = int((k^2)/2);
        int numSwitches_L3 = int((k/2)^2);
        
        int hostPerSwitch = int(k/2);
        
        int numHosts = int((k^3)/4);	// k [pods] * k/2 [L1/pod] * k/2 [hosts/L1]
        int numSwitches = numSwitches_L1 + numSwitches_L2 + numSwitches_L3;
        
        string load_balacing = default("random");

		switch_L1[*].relay.indexOffset = 0;
		switch_L2[*].relay.indexOffset = numSwitches_L1;
		switch_L3[*].relay.indexOffset = numSwitches_L2;
		
    submodules:
		
        switch_L1[numSwitches_L1]: Switch {
            @display("p=232,250,r,90;i=block/routing");
        }
        
        switch_L2[numSwitches_L2]: Switch {
            @display("p=232,150,r,90;i=block/routing");
        }
        
        switch_L3[numSwitches_L3]: Switch {
            @display("p=432,50,r,90;i=block/routing");
        }
        
        source[numHosts]: Source {
            @display("p=80,350,r,60;i=block/source_s");
                //*.eth[*].encapType = "EtherEncapCos";
        }

        sink[numHosts]: Sink {
            @display("p=90,350,r,60;i=block/sink_s");
                //*.eth[*].encapType = "EtherEncapCos";
        }
    
    connections:
        
        // for each pod, connect all L1 switches to the L2 switches
		for pod=0..k-1, for i=pod*hostPerSwitch..(pod+1)*hostPerSwitch-1, for j=pod*hostPerSwitch..(pod+1)*hostPerSwitch-1 {
		    switch_L1[i].out++ --> switch_L2[j].in++;
		    switch_L2[j].out++ --> switch_L1[i].in++;
		}
		
		// for each pod, connect all L2 switches to the L3 switches
		for i=0..numSwitches_L3-1, for j=0..numSwitches_L2-1 {
		    switch_L3[i].out++ --> switch_L2[j].in++;
		    switch_L2[j].out++ --> switch_L3[i].in++;
		}
		
		// rack cabling
        for pod=0..k-1, for i=pod*hostPerSwitch..(pod+1)*hostPerSwitch-1, for j=i*hostPerSwitch..(i+1)*hostPerSwitch-1 {
            source[j].out --> switch_L1[i].in++;
            switch_L1[i].out++ --> sink[j].in++;
        }
}
