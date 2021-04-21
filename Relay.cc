//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 1992-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include <omnetpp.h>
#include "CountMinSketch.h"
#include "packet_m.h"

using namespace omnetpp;

/**
 * Sends received packets to one of the outputs; see NED file for more info
 */
class Relay : public cSimpleModule
{
    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;

        virtual void forward(Packet* msg);
        virtual void perform_measure(Packet* msg, int n_hash);

    protected:
        CountMinSketch* cms;
        int switch_id;
};

Define_Module(Relay);

void Relay::initialize()
{
    cms = check_and_cast<CountMinSketch*>(getParentModule()->getSubmodule("sketch"));
    switch_id = getParentModule()->getIndex();
}

void Relay::handleMessage(cMessage *msg)
{
    Packet* pkt = check_and_cast<Packet*>(msg);
    int n_hash = pkt->getFlow().useSketch[switch_id];
    if (n_hash) {
        EV_INFO << "Updating sketch for packet #" << pkt->getFlow().seq << " of flow <" << pkt->getFlow().id << ">" << endl;
        perform_measure(pkt, n_hash);
    }
    forward(pkt);
}

void Relay::forward(Packet* pkt) {
    int size = pkt->getRouteArraySize();
    int outGate = pkt->getRoute(size-1);
    pkt->setRouteArraySize(size-1); // trim size
    send(pkt, "out", outGate);
}

void Relay::perform_measure(Packet* pkt, int n_hash) {
    const char* item = pkt->getFlow().id.c_str();

    // update fragment counting one packet
    cms->update(item, 1, n_hash);

    // set new minimimum if absent (-1) or better estimate
    unsigned int est = cms->estimate(item, n_hash);
    if (pkt->getMin() == -1 || pkt->getMin() > est) {
        pkt->setMin(est);
    }

}

