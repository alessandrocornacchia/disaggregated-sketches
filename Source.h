//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2006-2015 OpenSim Ltd.
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#ifndef __SOURCE_H
#define __SOURCE_H

#include <omnetpp.h>
#include <list>
#include <iterator>
#include "packet_m.h"

#define FLOW_ARRIVAL 0
#define PACKET_TX 1
#define MIN(a,b)  (a < b ? a : b)

typedef std::list<Flow>::iterator RoundRobinPtr;

using namespace omnetpp;
using namespace std;

/**
 * Abstract base class for job generator modules
 */
class Source : public cSimpleModule
{

    protected:

        enum SketchLoadBalance {RANDOM, DETERMINISTIC, SUICIDE};

        int flowCounter;
        int maxFlows;
        int numActiveFlows;
        string srcName;
        string dstName;
        list<Flow> activeFlows;
        RoundRobinPtr it;
        SketchLoadBalance lb;
        vector<int> used_fragments; // used if load balance policy is suicide
        int deterministic_load_balance_ptr = 0;

        /* two events can happen in the simulator */
        cMessage* flowArrivalMsg;
        cMessage* txTimeMsg;

        simtime_t startTime;
        simtime_t stopTime;

        static simsignal_t flowSizeSignal;
        simsignal_t createdSignal;

    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

        virtual void scheduleNextPacket();
        virtual Flow createFlow();
        virtual void txPacket();
        void route(Packet* pkt);

        void choose_fragments(Flow& f);
        vector<int> choose_K_at_random_without_repetition(int k);

    public:
        ~Source();
};

#endif


