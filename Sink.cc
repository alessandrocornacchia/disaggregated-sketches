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

simsignal_t Sink::flowSizeSignal = registerSignal("flowSize");
simsignal_t Sink::errorSignal = registerSignal("errors");
simsignal_t Sink::pktErrorSignal = registerSignal("pktErrors");

Define_Module(Sink);

void Sink::initialize()
{
//    lifeTimeSignal = registerSignal("lifeTime");
//    totalQueueingTimeSignal = registerSignal("totalQueueingTime");
//    queuesVisitedSignal = registerSignal("queuesVisited");
//    totalServiceTimeSignal = registerSignal("totalServiceTime");
//    totalDelayTimeSignal = registerSignal("totalDelayTime");
//    delaysVisitedSignal = registerSignal("delaysVisited");
//    generationSignal = registerSignal("generation");
//
    numFlows = 0;
}

void Sink::handleMessage(cMessage *msg)
{
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
        emit(Sink::flowSizeSignal, pkt->getFlow().size);
        emit(Sink::errorSignal, error);
    }
    // gather statistics
//    emit(lifeTimeSignal, simTime()- job->getCreationTime());
//    emit(totalQueueingTimeSignal, job->getTotalQueueingTime());
//    emit(queuesVisitedSignal, job->getQueueCount());
//    emit(totalServiceTimeSignal, job->getTotalServiceTime());
//    emit(totalDelayTimeSignal, job->getTotalDelayTime());
//    emit(delaysVisitedSignal, job->getDelayCount());
//    emit(generationSignal, job->getGeneration());

    delete msg;

    // end simulation when reached max Flows
    if (getIndex()==getVectorSize()-1 && numFlows == (int)par("maxFlows")) {
        endSimulation();
    }
}

void Sink::finish()
{
    // TODO missing scalar statistics
}


