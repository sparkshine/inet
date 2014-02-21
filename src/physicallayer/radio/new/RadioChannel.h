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

#ifndef __INET_RADIOCHANNEL_H_
#define __INET_RADIOCHANNEL_H_

#include <vector>
#include <algorithm>
#include "RadioChannelBase.h"
#include "IRadioBackgroundNoise.h"
#include "IRadioSignalAttenuation.h"

class INET_API RadioChannel : public RadioChannelBase, public XIRadioChannel
{
    protected:
        const double propagationSpeed;
        const double maximumCommunicationRange;
        const int mobilityApproximationCount;
        const IRadioBackgroundNoise *backgroundNoise;
        const IRadioSignalAttenuation *attenuation;
        std::vector<const XIRadio *> radios;
        std::vector<const IRadioSignalTransmission *> transmissions;

    protected:

        virtual simtime_t computeArrivalTime(simtime_t time, Coord position, IMobility *mobility) const;
        virtual bool isOverlappingTransmission(const IRadioSignalTransmission *transmission, const IRadioSignalReception *reception) const;

        virtual void eraseAllExpiredTransmissions();

        virtual const std::vector<const IRadioSignalTransmission *> *computeOverlappingTransmissions(const IRadioSignalReception *reception, const std::vector<const IRadioSignalTransmission *> *transmissions) const;
        virtual const std::vector<const IRadioSignalReception *> *computeOverlappingReceptions(const IRadioSignalReception *reception, const std::vector<const IRadioSignalTransmission *> *transmissions) const;
        virtual const IRadioDecision *computeDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission, const std::vector<const IRadioSignalTransmission *> *transmissions) const;

    public:
        RadioChannel() :
            propagationSpeed(SPEED_OF_LIGHT),
            maximumCommunicationRange(-1),
            mobilityApproximationCount(0),
            backgroundNoise(NULL),
            attenuation(NULL)
        {}

        RadioChannel(const IRadioBackgroundNoise *backgroundNoise, const IRadioSignalAttenuation *attenuation) :
            propagationSpeed(SPEED_OF_LIGHT),
            maximumCommunicationRange(-1),
            mobilityApproximationCount(0),
            backgroundNoise(backgroundNoise),
            attenuation(attenuation)
        {}

        virtual ~RadioChannel();

        virtual double getPropagationSpeed() const { return propagationSpeed; }
        virtual simtime_t computeTransmissionStartArrivalTime(const IRadioSignalTransmission *transmission, IMobility *mobility) const;
        virtual simtime_t computeTransmissionEndArrivalTime(const IRadioSignalTransmission *transmission, IMobility *mobility) const;

        virtual void addRadio(const XIRadio *radio) { radios.push_back(radio); }
        virtual void removeRadio(const XIRadio *radio) { radios.erase(std::remove(radios.begin(), radios.end(), radio)); }

        virtual void transmitSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission);
        virtual const IRadioDecision *receiveSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission) const;
        virtual bool isPotentialReceiver(const XIRadio *radio, const IRadioSignalTransmission *transmission) const;

        virtual void sendRadioFrame(XIRadio *radio, XIRadioFrame *frame);
};

#endif
