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
// A module that generates jobs. One can specify the number of jobs to be generated,
// the starting and ending time, and interval between generating jobs.
// Job generation stops when the number of jobs or the end time has been reached,
// whichever occurs first. The name, type and priority of jobs can be set as well.
//
simple Source
{
    parameters:
        @signal[created](type="long");
        @signal[flowSize](type="long");
        
        @statistic[created](title="the number of flows created";record=last;interpolationmode=none);
        @statistic[flowSize](title="Flow sizes"; record=vector);
        @display("i=block/source");
        
        int maxFlows = default(-1);               // number of flows to be generated (-1 means no limit)
        volatile double interArrivalTime @unit(s) = default(exponential(1s)); // time between generated flows
        volatile double flowSize = default(exponential(10));	// flow size distribution
        int maxFlowSize = default(10000);	// truncate flow size after this value 
        volatile double packetTxTime @unit(s) = default(1s);
        volatile string destination = "sink[0]";	// TODO not used now
        
        //volatile int jobType = default(0);       // the type attribute of the created job (used by classifers and other modules)
        //volatile int jobPriority = default(0);   // priority of the job
        double startTime @unit(s) = default(interArrivalTime); // when the module sends out the first job. -1 means deactivated
        double stopTime @unit(s) = default(-1s); // when the module stops the job generation (-1 means no limit)
    gates:
        output out;
}

