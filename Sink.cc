//
// This file is part of an OMNeT++/OMNEST simulation example.
//
// Copyright (C) 2006-2015 OpenSim Ltd.
//
// This file is distributed WITHOUT ANY WARRANTY. See the file
// `license' for details on this and other legal matters.
//

#include "Sink.h"
#include "packet_m.h"
#include "CountMinSketch.h"

simsignal_t Sink::flowSizeSignal = registerSignal("flowSize");
simsignal_t Sink::errorSignal = registerSignal("errors");
simsignal_t Sink::endFlowSizeSignal = registerSignal("endFlowSize");
simsignal_t Sink::endErrorSignal = registerSignal("endErrors");
simsignal_t Sink::pktErrorSignal = registerSignal("pktErrors");

Define_Module(Sink);

void Sink::initialize()
{
    numFlows = 0;
    if ((simtime_t)par("epochDuration") > SIMTIME_ZERO) {
        endEpoch = new cMessage("End measurement epoch");
        scheduleAt(simTime() + (simtime_t)par("epochDuration"), endEpoch);
        EV_INFO << "Measurement epoch ends at " << SIMTIME_STR(simTime() + (simtime_t)par("epochDuration")) << endl;
    }

}

void Sink::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // end of measurement interval: query sketches
        query_sketches();
        scheduleAt(simTime()+(simtime_t)par("epochDuration"), endEpoch);

    } else {

        Packet *pkt = check_and_cast<Packet *>(msg);

        // evaluate current error (i.e. sequence - estimation)
        int error = pkt->getMin() - pkt->getFlow().seq;

        EV_INFO << "Received packet #" << pkt->getFlow().seq << " of flow <" << pkt->getFlow().id << ">" << endl;
        EV_INFO << "Flow estimation has error " << error << endl;
        emit(Sink::pktErrorSignal, error);

        // at last packet collect stats
        if (pkt->getFlow().seq == pkt->getFlow().size) {
            EV_INFO << "Flow <" << pkt->getFlow().id << "> terminated, emitting statistics" << endl;
            numFlows++; // update number of terminated flows
            rxFlows.push_back(pkt->getFlow()); // store rx flow identifiers
            emit(Sink::flowSizeSignal, pkt->getFlow().size);
            emit(Sink::errorSignal, error);
        }

        delete msg;

        // if maxFlows is set (i.e., maxFlows > -1), end simulation when reached
        if ((int)par("maxFlows")>0 && numFlows == (int)par("maxFlows")) {

            // trigger all sinks to query for respective flows
            for (int i=0; i < getVectorSize(); i++) {
                string pathName = string("^.sink[") + to_string(i) + string("]");
                check_and_cast<Sink*>(getModuleByPath(pathName.c_str()))->query_sketches();
            }

            endSimulation();
        }
    }
}

// perform queries on sketches on the path for all rx flows
void Sink::query_sketches() {

    EV_INFO << "Querying all received flows and evaluating errors" << endl;

    while (!rxFlows.empty()) {  // for all received flows

        Flow f = rxFlows.front(); rxFlows.pop_front();
        unsigned int est = UINT_MAX;
        for (int i=0; i < f.useSketch.size(); i++) {
            if (f.useSketch[i]) {   // query fragments where flows where stored
                string pathName = string("^.switch[") + to_string(i) + string("].sketch");
                cModule* mod = getModuleByPath(pathName.c_str());
                CountMinSketch* cms = check_and_cast<CountMinSketch*>(mod);
                const char* item = f.id.c_str();
                unsigned local_est = cms->estimate(item);
                if (local_est < est) {
                    est = local_est;
                }
            }
        }

        emit(Sink::endFlowSizeSignal, f.size);
        emit(Sink::endErrorSignal, abs(est - f.size));

    }
}

void Sink::finish()
{
    cancelAndDelete(endEpoch);
}


