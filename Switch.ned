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

module Switch
{
    @display("bgb=523,203");
	@switch;
	
    gates:
        input in[];
        output out[];

    submodules:
        relay: Relay {
            @display("p=250,112;is=l");
        }
        sketch: CountMinSketch {
            @display("p=120,44");
        }

	connections:
	    for i=0..sizeof(in)-1 {
	    	in[i] --> relay.in++;
	    	relay.out++ --> out[i];
	    }
}
