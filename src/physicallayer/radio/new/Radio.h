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

#ifndef __INET_RADIO_H_
#define __INET_RADIO_H_

#include "IRadioChannel.h"
#include "IRadioAntenna.h"
#include "IRadioDecider.h"
#include "IRadioSignalTransmissionFactory.h"
#include "RadioBase.h"

// TODO: merge with RadioFrame
class INET_API XRadioFrame : public cPacket, public XIRadioFrame
{
    protected:
        const IRadioSignalTransmission *transmission;

    public:
        XRadioFrame(const IRadioSignalTransmission *transmission) :
            transmission(transmission)
        {}

        virtual const IRadioSignalTransmission *getTransmission() const { return transmission; }
};

// TODO: merge with RadioBase
class INET_API Radio : public RadioBase, public XIRadio
{
    protected:
        static unsigned int nextId;

    protected:
        const unsigned int id;

        const IRadioSignalTransmissionFactory *signalFactory;
        const IRadioAntenna *antenna;
        const IRadioDecider *decider;
        XIRadioChannel *channel;

    public:
        Radio() :
            id(nextId++),
            signalFactory(NULL),
            antenna(NULL),
            decider(NULL),
            channel(NULL)
        {}

        Radio(RadioMode radioMode, const IRadioSignalTransmissionFactory *signalFactory, const IRadioAntenna *antenna, const IRadioDecider *decider, XIRadioChannel *channel) :
            id(nextId++),
            signalFactory(signalFactory),
            antenna(antenna),
            decider(decider),
            channel(channel)
        {
            channel->addRadio(this);
        }

        virtual unsigned int getId() const { return id; }

        virtual const IRadioAntenna *getAntenna() const { return antenna; }
        virtual const IRadioDecider *getDecider() const { return decider; }
        virtual const XIRadioChannel *getXRadioChannel() const { return channel; }

        virtual XIRadioFrame *transmitPacket(cPacket *packet, simtime_t startTime);
        virtual cPacket *receivePacket(XIRadioFrame *frame);

        // TODO: delme
        virtual void handleMessageWhenUp(cMessage *msg) {}
};

#endif
