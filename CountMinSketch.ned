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

//
// TODO auto-generated module
//
simple CountMinSketch
{
    parameters:
        @display("i=block/table2");
        int d = default(-1);	// number hash
        int w = default(-1);	// number buckets
        double eps = default(0.01);
        double delta = default(0.01);
       	
       	@signal[packets];
       	@signal[counterLoad];
       	@signal[elementInserted];
       	
        @statistic[packets](title="number of packets processed"; record=count);
        @statistic[counterLoad](title="Average value on counter"; record=last);
        @statistic[elementInserted](record=sum);
        
}
