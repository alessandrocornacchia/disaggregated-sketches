//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 1992-2015 Andras Varga
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include "Relay.h"

using namespace omnetpp;

Define_Module(Relay);

void Relay::initialize()
{
    cms = check_and_cast<CountMinSketch*>(getParentModule()->getSubmodule("sketch"));
    switch_id = getParentModule()->getIndex() + (int)par("indexOffset");
    populateRoutingTables();
}

void Relay::handleMessage(cMessage *msg)
{
    Packet* pkt = check_and_cast<Packet*>(msg);

    EV_INFO << "Forwarding packet #" << pkt->getFlow().seq << " of flow <" << pkt->getFlow().id << ">";
    for (const auto& x : pkt->getFlow().FWI) {
        EV_INFO << x;
    }
    EV_INFO << endl;

    int fwi = pkt->getFlow().FWI.front();

    // measure flow if it has to
    if (fwi) {
        EV_INFO << "Updating sketch" << endl;
        pkt->getFlow().useSketch[switch_id] = getParentModule(); //fwi;
        perform_measure(pkt, fwi);
    }
    // remove control information and forward
    pkt->getFlow().FWI.pop_front();
    forward(pkt);
}

void Relay::forward(Packet* pkt) {
    int size = pkt->getRouteArraySize();
    int outGate = pkt->getRoute(size-1);
    pkt->setRouteArraySize(size-1); // trim size
    send(pkt, "out", outGate);
}

/* ask the sketch to update counters for this flow. n_hash specifies in how many
 * hash functions the memory have to be organized for this flow
 */
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

void Relay::populateRoutingTables() {

    inet::Topology *topo = new inet::Topology();
    routes.resize(getModuleByPath("sink[0]")->getVectorSize());

    // include in topology all sinks and switches (would be better to use @properties)
    topo->extractByModulePath(cStringTokenizer("**.sink* **.switch*").asVector());
    EV << "Topology found " << topo->getNumNodes() << " nodes\n";

    // switch containing this relay unit
    inet::Topology::Node *thisNode = topo->getNodeFor(this->getParentModule());

    // find and store routes to all other hosts
    for (int i = 0; i < topo->getNumNodes(); i++) {

        string nodeName = string(topo->getNode(i)->getModule()->getName());
        if (nodeName.find("sink") == string::npos) {
            continue;  // compute routes toward sink nodes only
        }

        topo->calculateUnweightedMultiShortestPathsTo(topo->getNode(i));

        if (thisNode->getNumPaths() == 0) {
            continue;  // not connected
        } else {    // add all output interfaces towards destination host

            EV << "Route for " << topo->getTargetNode()->getModule()->getFullName() << " has interfaces [";
            int destId = topo->getNode(i)->getModule()->getIndex();
            for (auto p : thisNode->getAllPaths()) {
                int outGateId = p->getLocalGate()->getIndex();
                routes[destId].push_back(p->getLocalGate());
                EV << outGateId;
            }
            EV << "]" << endl;
        }

    }
    delete topo;
}

vector<cGate*> Relay::getRouteTo(int dest) {
    return routes[dest];
}
