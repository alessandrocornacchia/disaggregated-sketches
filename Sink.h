//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2006-2015 OpenSim Ltd.
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#ifndef __SKETCHES_SINK_H
#define __SKETCHES_SINK_H

#include "omnetpp.h"
#include "packet_m.h"

using namespace omnetpp;
using namespace std;

/**
 * Consumes jobs; see NED file for more info.
 */
class Sink : public cSimpleModule
{
    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

        static simsignal_t flowSizeSignal;
        static simsignal_t errorSignal;
        static simsignal_t endFlowSizeSignal;
        static simsignal_t endErrorSignal;
        static simsignal_t pktErrorSignal;
        static simsignal_t usedSketchSignal;

    private:
        string sketch_vec_to_str(vector<int> v);
        long sketch_vec_to_long(vector<int> v);

    public:
        void query_sketches();

    protected:
        int numFlows;
        list<Flow> rxFlows;
        cMessage* endEpoch = nullptr; // end measurement epoch
};

#endif

