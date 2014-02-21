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

#ifndef __INET_IRADIOCHANNEL_H_
#define __INET_IRADIOCHANNEL_H_

#include "IRadio.h"
#include "IRadioFrame.h"
#include "IRadioDecision.h"

/**
 * This purely virtual interface provides an abstraction for different radio channels.
 */
class INET_API IRadioChannel
{
  public:
    virtual ~IRadioChannel() { }

    /**
     * Returns the number of available radio channels.
     */
    virtual int getNumChannels() = 0;
};

// TODO: merge with IRadioChannel
class INET_API XIRadioChannel
{
    public:
        virtual ~XIRadioChannel() {}

        virtual double getPropagationSpeed() const = 0;
        virtual simtime_t computeTransmissionStartArrivalTime(const IRadioSignalTransmission *transmission, IMobility *mobility) const = 0;
        virtual simtime_t computeTransmissionEndArrivalTime(const IRadioSignalTransmission *transmission, IMobility *mobility) const = 0;

        virtual void addRadio(const XIRadio *radio) = 0;
        virtual void removeRadio(const XIRadio *radio) = 0;

        virtual void transmitSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission) = 0;
        virtual const IRadioDecision *receiveSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission) const = 0;
        virtual bool isPotentialReceiver(const XIRadio *radio, const IRadioSignalTransmission *transmission) const = 0;

        virtual void sendRadioFrame(XIRadio *radio, XIRadioFrame *frame) = 0;
};

#endif
