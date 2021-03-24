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

package sketches;

// TODO correct positioning 
network ParkingLot
{
    parameters:
        int numHosts = default(10);
        int numSwitches = default(8);
        int K = default(numSwitches);
        string load_balacing = default("random");
       
    submodules:

        switch[numSwitches]: Switch {
            @display("p=232,208,r,90;i=block/routing");
        }
        source[numHosts]: Source {
            @display("p=110,308,r,90");
                //*.eth[*].encapType = "EtherEncapCos";
        }

        sink[numHosts]: Sink {
            @display("p=950,108,r,-90");
                //*.eth[*].encapType = "EtherEncapCos";
        }

    connections:

        // connect first source to first switch
        source[0].out --> switch[0].in++;

        // connect all switches in the middle
        for i=0..numSwitches-1 {
            
            // vertical connections
            source[i+1].out --> switch[i].in++; // source to switch
            switch[i].out++ --> sink[numHosts-1-i].in++;	// switch to sink
            
            // horizontal connections 
            switch[i].out++ --> switch[i+1].in++ if i < numSwitches-1;
            
        }

		// connect last switch to last sink
        switch[numSwitches-1].out++ --> sink[0].in++;
}
