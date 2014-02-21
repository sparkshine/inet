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

#ifndef __INET_CACHEDRADIOCHANNEL_H_
#define __INET_CACHEDRADIOCHANNEL_H_

#include "RadioChannel.h"

// TODO: add runtime configurable array and/or map based decision cache based on radio and transmission ids (use base offset)
class INET_API CachedRadioChannel : public RadioChannel
{
    protected:
        mutable long cacheGetCount;
        mutable long cacheHitCount;

        int baseTransmissionId;
// TODO: std::vector<IRadioReception *> cachedReceptions;
        std::vector<std::vector<const IRadioDecision *> > cachedDecisions;

    protected:
        virtual void finish();

        virtual const IRadioDecision *getCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission) const;
        virtual void setCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission, const IRadioDecision *decision);
        virtual void removeCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission);
        virtual void invalidateCachedDecisions(const IRadioSignalTransmission *transmission);
        virtual void invalidateCachedDecision(const IRadioDecision *decision);

    public:
        CachedRadioChannel() :
            RadioChannel(),
            cacheGetCount(0),
            cacheHitCount(0),
            baseTransmissionId(0)
        {}

        CachedRadioChannel(const IRadioBackgroundNoise *noise, const IRadioSignalAttenuation *attenuation) :
            RadioChannel(noise, attenuation),
            cacheGetCount(0),
            cacheHitCount(0),
            baseTransmissionId(0)
        {}

        virtual const IRadioDecision *receiveSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission) const;
};

#endif
