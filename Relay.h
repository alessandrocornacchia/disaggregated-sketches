//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2006-2015 OpenSim Ltd.
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#ifndef __SKETCHES_RELAY_H
#define __SKETCHES_RELAY_H

#include "omnetpp.h"
#include "packet_m.h"
#include "CountMinSketch.h"
#include <vector>
#include "inet/common/Topology.h"

using namespace omnetpp;
using namespace std;

/**
 * Consumes jobs; see NED file for more info.
 */
/**
 * Sends received packets to one of the outputs; see NED file for more info
 */
class Relay : public cSimpleModule
{
    protected:
        vector<vector<cGate*>> routes;     // sink -> [ interfaces ]

    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;

        virtual void forward(Packet* msg);
        virtual void populateRoutingTables();
        virtual void perform_measure(Packet* msg, int n_hash);

    protected:
        CountMinSketch* cms;
        int switch_id;

    public:
        vector<cGate*> getRouteTo(int dest);
};

#endif

