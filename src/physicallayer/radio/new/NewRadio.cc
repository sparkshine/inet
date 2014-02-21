//
// Copyright (C) 2013 OpenSim Ltd
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "NewRadio.h"
#include "NewRadioChannel.h"
#include "ModuleAccess.h"
#include "NodeOperations.h"
#include "NodeStatus.h"
#include "ScalarImplementation.h"

Define_Module(NewRadio);

NewRadio::NewRadio()
{
    endTransmissionTimer = NULL;
}

NewRadio::~NewRadio()
{
    cancelAndDelete(endTransmissionTimer);
    cancelAndDeleteEndReceptionTimers();
}

void NewRadio::cancelAndDeleteEndReceptionTimers()
{
    for (EndReceptionTimers::iterator it = endReceptionTimers.begin(); it!=endReceptionTimers.end(); ++it)
        cancelAndDelete(*it);
    endReceptionTimers.clear();
}

void NewRadio::initialize(int stage)
{
    RadioBase::initialize(stage);
    EV << "Initializing NewRadio, stage = " << stage << endl;
    if (stage == INITSTAGE_LOCAL)
    {
        endTransmissionTimer = new cMessage("endTransmission");
        channel = check_and_cast<XIRadioChannel *>(simulation.getModuleByPath("radioChannel"));
        channel->addRadio(this);
        antenna = new IsotropicRadioAntenna(RadioBase::getMobility());
        decider = new ScalarSNRRadioDecider(10);
        radioMode = IRadio::RADIO_MODE_RECEIVER;
        signalFactory = new ScalarRadioSignalTransmissionFactory(2E+6, 100, 1E-3, 2.4E+9, 1E+6);
    }
}

void NewRadio::setRadioMode(RadioMode newRadioMode)
{
    Enter_Method_Silent();
    if (radioMode != newRadioMode)
    {
        cancelAndDeleteEndReceptionTimers();
        EV << "Changing radio mode from " << getRadioModeName(radioMode) << " to " << getRadioModeName(newRadioMode) << ".\n";
        radioMode = newRadioMode;
        emit(radioModeChangedSignal, newRadioMode);
        updateTransceiverState();
    }
}

// TODO: obsolete
void NewRadio::setRadioChannel(int newRadioChannel)
{
    Enter_Method_Silent();
    if (radioChannel != newRadioChannel)
    {
        EV << "Changing radio channel from " << radioChannel << " to " << newRadioChannel << ".\n";
        radioChannel = newRadioChannel;
        emit(radioChannelChangedSignal, newRadioChannel);
    }
}

void NewRadio::handleMessageWhenUp(cMessage *message)
{
    if (message->isSelfMessage())
        handleSelfMessage(message);
    else if (message->getArrivalGate() == upperLayerIn)
    {
        if (!message->isPacket())
            handleUpperCommand(message);
        else if (radioMode == RADIO_MODE_TRANSMITTER || radioMode == RADIO_MODE_TRANSCEIVER)
            handleUpperFrame(check_and_cast<cPacket *>(message));
        else
        {
            EV << "Radio is not in transmitter or transceiver mode, dropping frame.\n";
            delete message;
        }
    }
    else if (message->getArrivalGate() == radioIn)
    {
        if (radioMode == RADIO_MODE_RECEIVER || radioMode == RADIO_MODE_TRANSCEIVER)
            handleLowerFrame(check_and_cast<XRadioFrame*>(message));
        else
        {
            EV << "Radio is not in receiver or transceiver mode, dropping frame.\n";
            delete message;
        }
    }
    else
    {
        throw cRuntimeError("Unknown arrival gate '%s'.", message->getArrivalGate()->getFullName());
        delete message;
    }
}

void NewRadio::handleUpperFrame(cPacket *packet)
{
    if (endTransmissionTimer->isScheduled())
        throw cRuntimeError("Received frame from upper layer while already transmitting.");
    XRadioFrame *radioFrame = check_and_cast<XRadioFrame *>(transmitPacket(packet, simTime()));
    channel->sendRadioFrame(this, radioFrame);
    EV << "Transmission of " << packet << " started\n";
    ASSERT(radioFrame->getDuration() != 0);
    scheduleAt(simTime() + radioFrame->getDuration(), endTransmissionTimer);
    updateTransceiverState();
}

void NewRadio::handleUpperCommand(cMessage *message)
{
}

void NewRadio::handleSelfMessage(cMessage *message)
{
    if (message == endTransmissionTimer) {
        EV << "Transmission successfully completed.\n";
        updateTransceiverState();
    }
    else
    {
        EV << "Frame is completely received now.\n";
        for (EndReceptionTimers::iterator it = endReceptionTimers.begin(); it != endReceptionTimers.end(); ++it)
        {
            if (*it == message)
            {
                endReceptionTimers.erase(it);
                XRadioFrame *radioFrame = check_and_cast<XRadioFrame *>(message->removeControlInfo());
                cPacket *macFrame = receivePacket(radioFrame);
                EV << "Sending up " << macFrame << ".\n";
                send(macFrame, upperLayerOut);
                updateTransceiverState();
                delete radioFrame;
                delete message;
                return;
            }
        }
        throw cRuntimeError("Self message not found in endReceptionTimers.");
    }
}

void NewRadio::handleLowerFrame(XRadioFrame *radioFrame)
{
    EV << "Reception of " << radioFrame << " started.\n";
    cMessage *endReceptionTimer = new cMessage("endReception");
    endReceptionTimer->setControlInfo(radioFrame);
    endReceptionTimers.push_back(endReceptionTimer);
    // NOTE: use arrivalTime instead of simTime, because we might be calling this
    // function during a channel change, when we're picking up ongoing transmissions
    // on the channel -- and then the message's arrival time is in the past!
    scheduleAt(radioFrame->getArrivalTime() + radioFrame->getDuration(), endReceptionTimer);
    updateTransceiverState();
}

bool NewRadio::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if (stage == NodeStartOperation::STAGE_PHYSICAL_LAYER)
            setRadioMode(RADIO_MODE_OFF);
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if (stage == NodeStartOperation::STAGE_PHYSICAL_LAYER)
            setRadioMode(RADIO_MODE_OFF);
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if (stage == NodeStartOperation::STAGE_LOCAL)
            setRadioMode(RADIO_MODE_OFF);
    }
    return true;
}

void NewRadio::updateTransceiverState()
{
    // reception state
    // TODO: correctly identify the reception state
    unsigned int interferenceCount = 0;
    ReceptionState newRadioReceptionState;
    unsigned int endReceptionCount = endReceptionTimers.size();
    if (radioMode == RADIO_MODE_OFF || radioMode == RADIO_MODE_SLEEP || radioMode == RADIO_MODE_TRANSMITTER)
        newRadioReceptionState = RECEPTION_STATE_UNDEFINED;
    else if (interferenceCount < endReceptionCount)
        newRadioReceptionState = RECEPTION_STATE_RECEIVING;
    else if (false) // NOTE: synchronization is not modeled in New radio
        newRadioReceptionState = RECEPTION_STATE_SYNCHRONIZING;
    else if (interferenceCount > 0)
        newRadioReceptionState = RECEPTION_STATE_BUSY;
    else
        newRadioReceptionState = RECEPTION_STATE_IDLE;
    if (receptionState != newRadioReceptionState)
    {
        EV << "Changing radio reception state from " << getRadioReceptionStateName(receptionState) << " to " << getRadioReceptionStateName(newRadioReceptionState) << ".\n";
        receptionState = newRadioReceptionState;
        emit(receptionStateChangedSignal, newRadioReceptionState);
    }
    // transmission state
    TransmissionState newRadioTransmissionState;
    if (radioMode == RADIO_MODE_OFF || radioMode == RADIO_MODE_SLEEP || radioMode == RADIO_MODE_RECEIVER)
        newRadioTransmissionState = TRANSMISSION_STATE_UNDEFINED;
    else if (endTransmissionTimer->isScheduled())
        newRadioTransmissionState = TRANSMISSION_STATE_TRANSMITTING;
    else
        newRadioTransmissionState = TRANSMISSION_STATE_IDLE;
    if (transmissionState != newRadioTransmissionState)
    {
        EV << "Changing radio transmission state from " << getRadioTransmissionStateName(transmissionState) << " to " << getRadioTransmissionStateName(newRadioTransmissionState) << ".\n";
        transmissionState = newRadioTransmissionState;
        emit(transmissionStateChangedSignal, newRadioTransmissionState);
    }
}
