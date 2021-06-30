//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2006-2015 OpenSim Ltd.
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include "Source.h"
#include "Relay.h"
#include "inet/common/Topology.h"
#include <sstream>
#include <numeric>
#include <algorithm>

simsignal_t Source::flowSizeSignal = registerSignal("flowSize");
simsignal_t Source::createdSignal = registerSignal("created");

Define_Module(Source);

/*
 * schedule first flow, set empty flow list, set round robin iterator to first flow
 */
void Source::initialize()
{

    flowCounter = 0;
    numActiveFlows = 0;
    numSwitches = getAncestorPar("numSwitches");

    startTime = par("startTime");
    stopTime = par("stopTime");
    maxFlows = par("maxFlows");
    srcName = this->getFullName();

    string lbpar = getAncestorPar("load_balacing").stdstringValue();
    std::transform(lbpar.begin(), lbpar.end(), lbpar.begin(), ::toupper);

    if (lbpar == "RANDOM") {
        lb = SketchLoadBalance::RANDOM;
    } else if (lbpar == "DETERMINISTIC") {
        lb = SketchLoadBalance::DETERMINISTIC;
    } else if (lbpar == "SUICIDE") {
        lb = SketchLoadBalance::SUICIDE;
    } else if (lbpar == "HEURISTIC") {
        lb = SketchLoadBalance::HEURISTIC;
    }

    topology = new cTopology("topology");
    // include in topology all sources, sinks and switches (would be better to use @properties)
    topology->extractByModulePath(cStringTokenizer("**.source* **.sink* **.switch*").asVector());

    //populateRoutingTable();

    flowArrivalMsg = new cMessage("flow arrival", FLOW_ARRIVAL);
    txTimeMsg = new cMessage("tx ended", PACKET_TX);

    // schedule the first message timer for start time
    // if start time is negative or 0 it's like deactivating the source
    if (startTime > 0)
        scheduleAt(startTime, flowArrivalMsg);

    WATCH(flowCounter);
    WATCH(numActiveFlows);
    //WATCH_LIST(activeFlows);

}

/* get shortest path routes toward all other hosts in the network */
void Source::populateRoutingTable() {

        cTopology *topo = new cTopology("topo");

        // include in topology all sources, sinks and switches (would be better to use @properties)
        //topo->extractByProperty("switch");
        topo->extractByModulePath(cStringTokenizer("**.source* **.sink* **.switch*").asVector());
        EV << "cTopology found " << topo->getNumNodes() << " nodes\n";

        cTopology::Node *thisNode = topo->getNodeFor(this);

        // find and store routes to all other hosts
        for (int i = 0; i < topo->getNumNodes(); i++) {

            string nodeName = string(topo->getNode(i)->getModule()->getName());
            if (nodeName.find("sink") == string::npos) {
                continue;  // compute routes toward sink nodes only
            }
            topo->calculateUnweightedSingleShortestPathsTo(topo->getNode(i));

            if (thisNode->getNumPaths() == 0) {
                continue;  // not connected
            }

            // source routing start computing routes from first hop in the network
            // i.e., source host output interface is not included
            cTopology::Node *currentNode = thisNode->getPath(0)->getRemoteNode();
            std::vector<int> routeToTarget;
            // walk through topology towards target node and collect list of forwarding interface indexes
            while (currentNode != topo->getTargetNode()) {

                cGate *outGate = currentNode->getPath(0)->getLocalGate();
                int gateIndex = outGate->getIndex();
                ASSERT(outGate->getOwnerModule() == currentNode->getModule());
                routeToTarget.push_back(gateIndex);
                currentNode = currentNode->getPath(0)->getRemoteNode();
            }

            int address = topo->getTargetNode()->getModule()->getId();
            rtable[address] = routeToTarget;
            EV << "Route towards " << topo->getTargetNode()->getModule()->getFullName() << " with address " << address << " gateIndex is {";
            for (int i=0; i < routeToTarget.size()-1; i++) {
              EV << routeToTarget[i] << '-';
            }
            EV << routeToTarget.back() << "}" << endl;

        }
        delete topo;
}

void Source::handleMessage(cMessage *msg)
{
    ASSERT(msg->isSelfMessage());

    if (msg->getKind() == FLOW_ARRIVAL){    // handle flow arrival: create new flow and add to list of active flows
        if ((maxFlows < 0 || maxFlows > flowCounter) && (stopTime < 0 || stopTime > simTime())) {

            Flow f = createFlow();
            flowCounter++;

            // first flow
            if (activeFlows.empty()) {
                activeFlows.push_front(f);
                it = activeFlows.begin();
                txPacket();
            } else {
                activeFlows.push_front(f);
            }
            numActiveFlows = activeFlows.size();    // WATCH
            // reschedule the timer for the next flow
            scheduleAt(simTime() + par("interArrivalTime").doubleValue(), flowArrivalMsg);
        }
    } else if (msg->getKind() == PACKET_TX){    // handle new packet transmission
        if (!activeFlows.empty()) { // if there are flows available go on, otherwise wait for new flow
            txPacket();
        }
    }

}

void Source::txPacket() {

    it->seq = it->seq + 1;  // count new packet sent

    Flow f = *it;   // now take flow
    Packet* pkt = new Packet(); // create packet and fill with flow and route
    pkt->setFlow(f);
    route2(pkt);

    EV_INFO << "Tx packet #" << f.seq << " of flow <" << f.id << ">" << endl;
    send(pkt, "out");

    scheduleNextPacket();
}

/* Schedule next packet to transmit. Packet is transmitted when the channel
 * becomes idle.
 * The packet will belong to the next flow among those active
 * in a circular Round Robin scheduling order.
 *
*/
void Source::scheduleNextPacket() {
    if (it->seq == it->size) {  // if flow is ended, delete it
        it = activeFlows.erase(it);
        numActiveFlows = activeFlows.size();
        // since when removing with erase updates the iterator, it might be that
        // iterator comes at end(), in this case we just restart from begin()
        if (it == activeFlows.end()) {
            it = activeFlows.begin();
        }
    } else { // increase pointer in circular manner
        if(next(it) == activeFlows.end()) { // XXX removed if(activeFlows.empty() || next(it) == activeFlows.end())
            it = activeFlows.begin();
        } else {
            it++;
        }
    }
    // schedule new packet only if not empty
    if (!activeFlows.empty()) {
        scheduleAt(simTime() + (simtime_t)par("packetTxTime"), txTimeMsg);
    }
}

/* reads routing table */
vector<int> Source::getRouteTo(int dst) {
    RoutingTable::iterator it = rtable.find(dst);
    if (it == rtable.end()) {
        string msg = string("Destination ") + to_string(dst) + string(" not found");
        throw cRuntimeError(msg.c_str());
    }
    return (*it).second;
}


/* route packet of given flow */
void Source::route(Packet* pkt) {

    vector<int> outGates = getRouteTo(pkt->getFlow().dst);
    pkt->setRouteArraySize(outGates.size());

    // add forwarding interfaces (setRoute requires reverse order because
    // we will trim size with setArraySize at relay units, cutting out last elements)
    for(int i=0; i < outGates.size(); i++){
        pkt->setRoute(outGates.size()-1-i, outGates[i]);
    }

}

/* Fat-Tree route */
void Source::route2(Packet* pkt) {
//        vector<int> outGates;
//        int destination = pkt->getFlow().dstIndex;
//
//        // start from first switch
//        cTopology::Node* currentNode = topology->getNodeFor(this)->getLinkOut(0)->getRemoteNode();
//        while (currentNode->getNumOutLinks() == 0) {    // finchè non siamo arrivati alla sink
//
//            Relay *relay = check_and_cast<Relay*>(currentNode->getModule()->getSubmodule("relay"));
//            vector<int> interfaces = relay->getRouteTo(destination);
//            int outIf = ecmp(interfaces, pkt->getFlow());
//            outGates.push_back(outIf);
//            currentNode = currentNode->getLinkOut(outIf)->getRemoteNode();
//
//        }
//        return outGates;
    vector<int> r = pkt->getFlow().SRI;
    pkt->setRouteArraySize(r.size());
    // add forwarding interfaces (setRoute requires reverse order because
    // we will trim size with setArraySize at relay units, cutting out last elements)
    for(int i=0; i < r.size(); i++){
        pkt->setRoute(r.size()-1-i, r[i]);
    }
}

int Source::ecmp(vector<int> interfaces, Flow& f) {
    return interfaces[(f.src ^ f.dst ^ f.app) % interfaces.size()];
}

/*
 * Chooses a subset of fragments at random among those available
 * on flow path.
 */
void Source::chooseFragments(Flow& f) {

    int numHop = f.SRI.size();
    int k = MIN((int)getAncestorPar("K"), numHop);

    // switches along the path will fill this
    f.useSketch.assign(numSwitches, nullptr); // 0 instead of nullptr

    switch (lb) {   // depending on lb policy

        case RANDOM:
            // use K at random
            f.FWI = randomSubset(k, numHop);
            break;

        case DETERMINISTIC: { // deterministic load balancing
            int from = deterministic_load_balance_ptr;
            int to = (deterministic_load_balance_ptr + k) % numHop;
            EV_INFO << "Using fragments from " << from << " to " << to << endl;
            f.FWI.assign(numHop, 0); // init to all zero

            for (unsigned i=from; i != to; i=(i+1) % numHop) {  // use fragments in range from-to with circular iteration
                f.FWI[i] = 1;
            }

            deterministic_load_balance_ptr = (deterministic_load_balance_ptr + k) % numHop;
            break;
        }

        case SUICIDE:
            // use always first k
            f.FWI.assign(numHop, 0);
            for (int i=0; i < k; i++) {
                f.FWI[i] = 1;
            }
            break;

        case HEURISTIC:
            f.FWI = heu1(numHop);
            break;

        default:
            break;
    }
}

/* extract k elements without repetition out of n */
deque<int> Source::randomSubset(int k, int n) {

    vector<int> fragments(n); // init to range starting from zero
    std::iota(fragments.begin(), fragments.end(), 0);
    deque<int> choice(n, 0);

    int idx;

    for (unsigned i=0; i < k; i++){
        idx = intuniform(0, fragments.size()-1);
        choice[fragments[idx]] = (int)par("numSketchFragments");
        fragments.erase(fragments.begin()+idx);
    }
    return choice;
}

deque<int> Source::heu1(int n) {
    deque<int> choice(n, 0); // init to range starting from zero
    int nf = (int)par("numSketchFragments");
    if (n==1) {
        choice[0] = nf;
    } else if (n == 3) {
        choice[0] = nf;
        choice[1] = nf;
        choice[2] = nf;
    } else if (n==5) {
        choice[1] = nf;
        choice[2] = nf;
        choice[3] = nf;
    } else {
        throw cRuntimeError("Number of hops not supported");
    }
    return choice;
}

Flow Source::createFlow()
{
    long fs = MIN((long)par("maxFlowSize"), ceil(par("flowSize").doubleValue()));  // add new flow to the list of active flows
    ASSERT(fs > 0);

    // extract random uniform destination
    string dstName = par("dstHost").stdstringValue();
    string fid = srcName + ":" + dstName + ":" + to_string(flowCounter); // flow identifier

    struct Flow f;
    f.size = fs;
    f.id = fid;
    f.src = getId();
    f.dst = getModuleByPath(dstName.c_str())->getId();
    f.dstIndex = getModuleByPath(dstName.c_str())->getIndex();
    f.app = intuniform(0, 65536-1);

    ///////////////////////////// TODO move in dedicated function
    cTopology::Node* currentNode = topology->getNodeFor(this)->getLinkOut(0)->getRemoteNode();
    while (currentNode->getNumOutLinks() != 0) {    // finchè non siamo arrivati alla sink

        Relay *relay = check_and_cast<Relay*>(currentNode->getModule()->getSubmodule("relay"));
        vector<int> interfaces = relay->getRouteTo(f.dstIndex);
        int outIf = interfaces[intuniform(0, interfaces.size()-1)];
        //ecmp(interfaces, f);
        f.SRI.push_back(outIf);
        currentNode = currentNode->getLinkOut(outIf)->getRemoteNode();

    }
    ///////////////////////////

    chooseFragments(f);

    EV_INFO << "Generated new flow <" << f.id << "> of size " << f.size << endl;
    emit(Source::flowSizeSignal, f.size);

    return f;

}

void Source::finish()
{
    cancelAndDelete(flowArrivalMsg);
    cancelAndDelete(txTimeMsg);
    emit(createdSignal, flowCounter);

    delete topology;
}

Source::~Source()
{
/*cancelAndDelete(flowArrivalMsg);
    cancelAndDelete(txTimeMsg);*/

}


