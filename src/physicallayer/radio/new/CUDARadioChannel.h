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

#ifndef __INET_CUDARADIOCHANNEL_H_
#define __INET_CUDARADIOCHANNEL_H_

#include "CachedRadioChannel.h"

typedef int64_t cuda_simtime_t;

class INET_API CUDARadioChannel : public CachedRadioChannel
{
    protected:
        virtual void computeCache(const std::vector<const XIRadio *> *radios, const std::vector<const IRadioSignalTransmission *> *transmissions);

    public:
        CUDARadioChannel() :
            CachedRadioChannel()
        {}

        CUDARadioChannel(IRadioBackgroundNoise *noise, IRadioSignalAttenuation *attenuation) :
            CachedRadioChannel(noise, attenuation)
        {}

        virtual void transmitSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission);
        virtual const IRadioDecision *receiveSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission) const;
};

#endif
