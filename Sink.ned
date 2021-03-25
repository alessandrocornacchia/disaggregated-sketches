//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2006-2015 OpenSim Ltd.
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

package sketches;

//
// Destroys (or optionally keep) the packets and collects statistics
//
simple Sink
{
    parameters:
        
        @display("i=block/sink");
        
        int maxFlows = default(-1);
        double epochDuration @unit(s) = default(0s);	// set to an interval if needed
        
//        @signal[lifeTime](type="simtime_t");
//        @signal[totalQueueingTime](type="simtime_t");
//        @signal[totalDelayTime](type="simtime_t");
//        @signal[totalServiceTime](type="simtime_t");
//        @signal[queuesVisited](type="long");
//        @signal[delaysVisited](type="long");
//        @signal[generation](type="long");
//        @statistic[lifeTime](title="lifetime of arrived jobs";unit=s;record=vector,mean,max;interpolationmode=none);
//        @statistic[totalQueueingTime](title="the total time spent in queues by arrived jobs";unit=s;record=vector?,mean,max;interpolationmode=none);
//        @statistic[totalDelayTime](title="the total time spent in delay nodes by arrived jobs";unit=s;record=vector?,mean,max;interpolationmode=none);
//        @statistic[totalServiceTime](title="the total time spent  by arrived jobs";unit=s;record=vector?,mean,max;interpolationmode=none);
//        @statistic[queuesVisited](title="the total number of queues visited by arrived jobs";record=vector?,mean,max;interpolationmode=none);
//        @statistic[delaysVisited](title="the total number of delays visited by arrived jobs";record=vector?,mean,max;interpolationmode=none);
//        @statistic[generation](title="the generation of the arrived jobs";record=vector?,mean,max;interpolationmode=none);
//        bool keepJobs = default(false); // whether to keep the received jobs till the end of simulation
		
		@signal[flowSize](type=long);
		@signal[errors](type=long);
		@signal[endFlowSize](type=long);
		@signal[endErrors](type=long);
		@signal[pktErrors](type=long);
		
		@statistic[flowSize](title="Flow sizes"; record=vector);
		@statistic[errors](title="Estimation errors"; record=vector);
		@statistic[endFlowSize](title="Flow sizes at end of measurement epoch"; record=vector);
		@statistic[endErrors](title="Estimation errors at end of measurement epoch"; record=vector);
		@statistic[pktErrors](title="Estimation errors packet by packet"; record=vector);
		@statistic[pktsRx](title="Number of received packets"; source=flowSize; record=sum); // number of packets received
		
		
    gates:
        input in[];
}

