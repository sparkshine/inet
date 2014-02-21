//
// Copyright (C) 2013 OpenSim Ltd.
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

#include "Radio.h"
#include "ImplementationBase.h"

unsigned int Radio::nextId = 0;

XIRadioFrame *Radio::transmitPacket(cPacket *packet, simtime_t startTime)
{
    const IRadioSignalTransmission *transmission = signalFactory->createTransmission(this, packet, startTime);
    channel->transmitSignal(this, transmission);
    XRadioFrame *radioFrame = new XRadioFrame(transmission);
    radioFrame->setDuration(transmission->getDuration());
    radioFrame->encapsulate(packet);
    return radioFrame;
}

cPacket *Radio::receivePacket(XIRadioFrame *frame)
{
    const IRadioDecision *radioDecision = channel->receiveSignal(this, frame->getTransmission());
    cPacket *packet = check_and_cast<cPacket *>(frame)->decapsulate();
    packet->setKind(radioDecision->isSuccessful());
    packet->setControlInfo(const_cast<cObject *>(check_and_cast<const cObject *>(check_and_cast<const RadioDecision *>(radioDecision))));
    return packet;
}
