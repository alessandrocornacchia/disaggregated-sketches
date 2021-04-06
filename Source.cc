//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2006-2015 OpenSim Ltd.
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include "Source.h"
#include <sstream>
#include <numeric>
#include <algorithm>

simsignal_t Source::flowSizeSignal = registerSignal("flowSize");

Define_Module(Source);

/*
 * schedule first flow, set empty flow list, set round robin iterator to first flow
 */
void Source::initialize()
{
    createdSignal = registerSignal("created");
    flowCounter = 0;
    numActiveFlows = 0;

    startTime = par("startTime");
    stopTime = par("stopTime");
    maxFlows = par("maxFlows");
    srcName = this->getFullName();
    dstName = par("destination").stdstringValue();

    string lbpar = getAncestorPar("load_balacing").stdstringValue();
    transform(lbpar.begin(), lbpar.end(), lbpar.begin(), ::toupper);

    if (lbpar == "RANDOM") {
        lb = SketchLoadBalance::RANDOM;
    } else if (lbpar == "DETERMINISTIC") {
        lb = SketchLoadBalance::DETERMINISTIC;
    } else if (lbpar == "SUICIDE") {
        lb = SketchLoadBalance::SUICIDE;
        used_fragments = choose_K_at_random_without_repetition((int)getAncestorPar("K"));
    }

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
    route(pkt);

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

void Source::route(Packet* pkt) {
    // compute routing
    if (!this->isVector()) {
        throw cRuntimeError("Not supported");
    }

    if (this->getIndex() == 0) {    // if source zero go horizontal
        pkt->setRouteArraySize(getVectorSize()-1);
        for(int i=0; i<getVectorSize()-1; i++){
            pkt->setRoute(i, 1);    // always use interface 1
        }
    } else { // go vertical
        pkt->setRouteArraySize(getVectorSize()-1);
        pkt->setRoute(0, 0);
    }
}

/*
 * Chooses a subset of fragments at random among those available
 * on flow path.
 */
void Source::choose_fragments(Flow& f) {

    int ns = getVectorSize() - 1;

    //f.useSketch.assign(ns, 0); // init to all zero

    if (this->getIndex() == 0) {    // horizontal flows
        int k = (int)getAncestorPar("K");

        if (k == ns) {  // DISCO
            f.useSketch.assign(ns, 1);
        } else {    // need of load balancing

            switch (lb) {   // depending on lb policy

                case RANDOM:
                    // use K at random
                    f.useSketch = choose_K_at_random_without_repetition(k);
                    break;

                case DETERMINISTIC: { // deterministic load balancing
                    int from = deterministic_load_balance_ptr;
                    int to = (deterministic_load_balance_ptr + k) % ns;
                    EV_INFO << "Using fragments from " << from << " to " << to << endl;
                    f.useSketch.assign(ns, 0); // init to all zero

                    for (unsigned i=from; i != to; i=(i+1) % ns) {  // use fragments in range from-to with circular iteration
                        f.useSketch[i] = 1;
                    }

                    deterministic_load_balance_ptr = (deterministic_load_balance_ptr + k) % ns;
                    break;
                }

                case SUICIDE:   // use always the same
                    f.useSketch = used_fragments;
                    break;

                default:
                    break;
            }
        }

    } else {    // vertical flows
        f.useSketch.assign(ns, 0); // init to all zero
        f.useSketch[getIndex()-1] = 1; // set one in correspondence of the switch traversed
    }
}

vector<int> Source::choose_K_at_random_without_repetition(int k) {

    // switches in this configuration are always one less than source vector
    int ns = getVectorSize() - 1;

    vector<int> fragments(ns); // init to range starting from zero
    std::iota(fragments.begin(), fragments.end(), 0);
    vector<int> choice(ns, 0);

    int idx;

    for (unsigned i=0; i<k; i++){
        idx = intuniform(0, fragments.size()-1);
        choice[fragments[idx]] = 1;
        fragments.erase(fragments.begin()+idx);
    }
    return choice;
}

Flow Source::createFlow()
{
    long fs = MIN((long)par("maxFlowSize"), ceil(par("flowSize").doubleValue()));  // add new flow to the list of active flows

    // inverse-transform method from uniform r.v.

    /* Pareto Type-I
    double scale = 1.6666666666666667;
    double alpha = 1.2;

    double U = uniform(0,1,1);  // use RNG with logical index 1
    double fsd = scale * pow(1.-U, -1./alpha);
    long fs = ceil(fsd); */

    ASSERT(fs > 0);

    //cModule* dstMod = getModuleByPath(dstName);
    //string fid = to_string(dstMod->getId()) + ":" + to_string(this->getId()) + ":" + to_string(flowCounter); // flow identifier

    string fid = srcName + ":" + dstName + ":" + to_string(flowCounter); // flow identifier

    struct Flow f;
    f.size = fs;
    f.id = fid;
    f.src = srcName;
    f.dst = dstName;

    choose_fragments(f);

    EV_INFO << "Generated new flow <" << f.id << "> of size " << f.size << endl;
    emit(Source::flowSizeSignal, f.size);

    return f;

}

void Source::finish()
{
    cancelAndDelete(flowArrivalMsg);
    cancelAndDelete(txTimeMsg);
    emit(createdSignal, flowCounter);
}

Source::~Source()
{
/*cancelAndDelete(flowArrivalMsg);
    cancelAndDelete(txTimeMsg);*/

}


