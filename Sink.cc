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

simsignal_t Sink::numHopSignal = registerSignal("traversedSwitches");
simsignal_t Sink::usedSketchSignal = registerSignal("usedSketches");
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

    // for all specified threshold build objects to store false positive rates
    const char *vstr = par("heavyHitterThresholds").stringValue();

    vhht = cStringTokenizer(vstr).asIntVector();
    for (auto hht : vhht)
    {
        char signalName[32];
        sprintf(signalName, "hht%d-efpr", hht);
        char statisticName[32];
        strcpy(statisticName, signalName);

        // dynamically add the signal to this cModule
        simsignal_t signal = registerSignal(signalName);
        cProperty *statisticTemplate = getProperties()->get("statisticTemplate", "efpr");
        getEnvir()->addResultRecorders(this, signal, statisticName, statisticTemplate);

        // first sink also add the signal recording to the network (root)
        if (getIndex() == 0) {
            statisticTemplate = getModuleByPath("<root>")->getProperties()->get("statisticTemplate", "efpr");
            getEnvir()->addResultRecorders(getModuleByPath("<root>"), signal, statisticName, statisticTemplate);
        }

        // store simsignal_t reference
        efprs.push_back(signal);

        // do the same for the signal for upper bound
        sprintf(signalName, "hht%d-ubfpr", hht);
        strcpy(statisticName, signalName);
        simsignal_t signal1 = registerSignal(signalName);
        statisticTemplate = getProperties()->get("statisticTemplate", "ubfpr");
        getEnvir()->addResultRecorders(this, signal1, statisticName, statisticTemplate);
        if (getIndex() == 0) {
            statisticTemplate = getModuleByPath("<root>")->getProperties()->get("statisticTemplate", "ubfpr");
            getEnvir()->addResultRecorders(getModuleByPath("<root>"), signal1, statisticName, statisticTemplate);
        }

        ubfprs.push_back(signal1);
    }

//    while (tokenizer.hasMoreTokens()) {
//
//        const char *token = tokenizer.nextToken();
//
//        vhht.push_back(atoi(token));
//
//        cStdDev stats_ubfpr = cStdDev((string("fpr-bound-") + token).c_str());
//        ubfpr.push_back(stats_ubfpr);
//
//        cStdDev stats_efpr = cStdDev((string("efpr-") + token).c_str());
//        efpr.push_back(stats_efpr);
//
//    }

}

void Sink::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // end of measurement interval: query sketches
        query_sketches();
        scheduleAt(simTime()+(simtime_t)par("epochDuration"), endEpoch);

    } else {

        Packet *pkt = check_and_cast<Packet *>(msg);

        if (pkt->getFlow().dst != this->getId()) {
            throw cRuntimeError("Unwanted reception: packet destination doesn't correspond to sink address");
        }

        // evaluate current error (i.e. sequence - estimation)
        int error = pkt->getMin() - pkt->getFlow().seq;

        EV_INFO << "Received packet #" << pkt->getFlow().seq << " of flow <" << pkt->getFlow().id << ">" << endl;
        EV_INFO << "Current flow size estimation error: " << error << endl;
        EV_INFO << "Flow has been recorded in switches: ";
        for (auto x: pkt->getFlow().useSketch) {
            if (x) {
                EV_INFO << to_string(((cModule*)x)->getIndex()) << ".";
            }
        }
        EV_INFO << endl;
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

            // trigger all sinks to query for respective flows (not implemented in finish() because
            // query_string invokes methods of sketch modules which might have been already destroyed)
            for (int i=0; i < getVectorSize(); i++) {
                string pathName = string("^.sink[") + to_string(i) + string("]");
                check_and_cast<Sink*>(getModuleByPath(pathName.c_str()))->query_sketches();
            }

            endSimulation();
        }
    }
}

/*
 * represents vector as a tokenized string
 */
string Sink::sketch_vec_to_str(vector<int> v) {
    string s;
    for (auto x : v) {
        s += (s.empty() ? "" : ".") + to_string(x);
    }
    return s;
}

/*
 * convert binary sketch vector to long in order to be recorded
 * as standard statistic
 */
long Sink::sketch_vec_to_long(vector<int> v) {
    long res = 0;
    for (int i=0; i < v.size(); i++) {
        if (v[i] > 1 || v[i] < 0) {
            throw cRuntimeError("Conversion supported only for binary vectors");
        }
        res += v[i] * pow(2, v.size()-1-i);
    }
    EV_DEBUG << sketch_vec_to_str(v) << " -> " << res << endl;
    return res;
}

// perform queries on sketches on the path for all rx flows
void Sink::query_sketches() {

    while (!rxFlows.empty()) {  // for all received flows

        Flow f = rxFlows.front();
        rxFlows.pop_front();

        unsigned int est = UINT_MAX;
        for (int i=0; i < f.useSketch.size(); i++) {
            if (f.useSketch[i]) {   // query fragments where flows where stored
                //string pathName = string("^.switch[") + to_string(i) + string("].sketch");
                //cModule* mod = getModuleByPath(pathName.c_str());
                cModule* mod = ((cModule*)f.useSketch[i])->getSubmodule("sketch");
                CountMinSketch* cms = check_and_cast<CountMinSketch*>(mod);
                const char* item = f.id.c_str();
                unsigned local_est = cms->estimate(item, 1); //f.useSketch[i]);
                if (local_est < est) {
                    est = local_est;
                }
            }
        }

        // for all threshold evaluate fpr bound and fpr
        for (int i=0; i < vhht.size(); i++)
        {
            EV_INFO << "Heavy-hitter threshold: " << to_string(vhht[i]) << endl;
            if (f.size < vhht[i]) // if non heavy hitter
            {
                double pfp = 1; // false positive probability
                for (int j=0; j < f.useSketch.size(); j++)
                {
                    if (f.useSketch[j]) // for all used sketches along the path evaluate formula
                    {
                        cModule* mod = ((cModule*)f.useSketch[j])->getSubmodule("sketch");
                        CountMinSketch* cms = check_and_cast<CountMinSketch*>(mod);
                        pfp = pfp * cms->normalizedtotalcount() / (vhht[i] - f.size); // formula
                    }
                }
                //ubfpr[i].collect(MIN(pfp, 1)); // clip to 1 if not a probability
                emit(ubfprs[i], MIN(pfp, 1));

                // for each flow collect 1 when false positive, then the mean will be the EFPR
                //efpr[i].collect(est >= vhht[i]);
                emit(efprs[i], est >= vhht[i]);
            }
        }

        emit(Sink::endFlowSizeSignal, f.size);
        emit(Sink::endErrorSignal, est - f.size);
        vector<int> v;
        for (auto x: f.useSketch) {
            v.push_back(x ? 1 : 0);
        }
        emit(Sink::usedSketchSignal, sketch_vec_to_long(v));
        emit(Sink::numHopSignal, f.SRI.size());

    }


}

void Sink::finish()
{
//    for (int i=0; i < ubfpr.size(); i++)
//    {
//        ubfpr[i].record();
//        efpr[i].record();
//    }
    cancelAndDelete(endEpoch);
}


