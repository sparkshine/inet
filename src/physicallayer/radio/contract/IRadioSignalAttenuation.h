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

#ifndef __INET_IRADIOSIGNALATTENUATION_H_
#define __INET_IRADIOSIGNALATTENUATION_H_

#include "IRadio.h"
#include "IRadioSignalTransmission.h"
#include "IRadioSignalReception.h"
#include "IRadioSignalLoss.h"

class INET_API IRadioSignalAttenuation
{
    public:
        virtual ~IRadioSignalAttenuation() {}

        virtual const IRadioSignalLoss *computeLoss(const XIRadio *radio, const IRadioSignalTransmission *transmission, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) const = 0;
        virtual const IRadioSignalReception *computeReception(const XIRadio *radio, const IRadioSignalTransmission *transmission) const = 0;
};

#endif
