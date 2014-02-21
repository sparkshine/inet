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

#include "IdealImplementation.h"
#include "IRadioChannel.h"

const IRadioSignalReception *IdealRadioSignalAttenuationBase::computeReception(const XIRadio *receiverRadio, const IRadioSignalTransmission *transmission) const
{
    const XIRadioChannel *radioChannel = receiverRadio->getXRadioChannel();
    const IRadioAntenna *receiverAntenna = receiverRadio->getAntenna();
    IMobility *receiverAntennaMobility = receiverAntenna->getMobility();
    simtime_t receptionStartTime = radioChannel->computeTransmissionStartArrivalTime(transmission, receiverAntennaMobility);
    simtime_t receptionEndTime = radioChannel->computeTransmissionEndArrivalTime(transmission, receiverAntennaMobility);
    Coord receptionStartPosition = receiverAntennaMobility->getPosition(receptionStartTime);
    Coord receptionEndPosition = receiverAntennaMobility->getPosition(receptionEndTime);
    IdealRadioSignalLoss::Factor attenuationFactor = check_and_cast<const IdealRadioSignalLoss *>(computeLoss(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition))->getFactor();
    IdealRadioSignalReception::Power power;
    switch (attenuationFactor)
    {
        case IdealRadioSignalLoss::FACTOR_WITHIN_COMMUNICATION_RANGE:
            power = IdealRadioSignalReception::POWER_RECEIVABLE; break;
        case IdealRadioSignalLoss::FACTOR_WITHIN_INTERFERENCE_RANGE:
            power = IdealRadioSignalReception::POWER_INTERFERING; break;
        case IdealRadioSignalLoss::FACTOR_OUT_OF_INTERFERENCE_RANGE:
            power = IdealRadioSignalReception::POWER_UNDETECTABLE; break;
        default:
            throw cRuntimeError("Unknown attenuation factor");
    }
    return new IdealRadioSignalReception(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition, power);
}

const IRadioSignalLoss *IdealRadioSignalFreeSpaceAttenuation::computeLoss(const XIRadio *radio, const IRadioSignalTransmission *transmission, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) const
{
    const IdealRadioSignalTransmission *idealTransmission = check_and_cast<const IdealRadioSignalTransmission *>(transmission);
    double distance = transmission->getStartPosition().distance(startPosition);
    IdealRadioSignalLoss::Factor factor;
    if (distance <= idealTransmission->getMaximumCommunicationRange())
        factor = IdealRadioSignalLoss::FACTOR_WITHIN_COMMUNICATION_RANGE;
    else if (distance <= idealTransmission->getMaximumInterferenceRange())
        factor = IdealRadioSignalLoss::FACTOR_WITHIN_INTERFERENCE_RANGE;
    else
        factor = IdealRadioSignalLoss::FACTOR_OUT_OF_INTERFERENCE_RANGE;
    return new IdealRadioSignalLoss(factor);
}

const IRadioDecision *IdealRadioDecider::computeDecision(const IRadioSignalReception *candidateReception, const std::vector<const IRadioSignalReception *> *overlappingReceptions, const IRadioSignalNoise *backgroundNoise) const
{
    const IdealRadioSignalReception::Power power = check_and_cast<const IdealRadioSignalReception *>(candidateReception)->getPower();
    if (power == IdealRadioSignalReception::POWER_RECEIVABLE)
    {
        if (ignoreInterference)
            return new RadioDecision(candidateReception, true);
        else
        {
            for (std::vector<const IRadioSignalReception *>::const_iterator it = overlappingReceptions->begin(); it != overlappingReceptions->end(); it++)
            {
                const IRadioSignalReception *overlappingReception = *it;
                IdealRadioSignalReception::Power overlappingPower = check_and_cast<const IdealRadioSignalReception *>(overlappingReception)->getPower();
                if (overlappingPower != IdealRadioSignalReception::POWER_UNDETECTABLE)
                    return new RadioDecision(candidateReception, false);
            }
            return new RadioDecision(candidateReception, true);
        }
    }
    else
        return new RadioDecision(candidateReception, false);
}

const IRadioSignalTransmission *IdealRadioSignalTransmissionFactory::createTransmission(const XIRadio *radio, const cPacket *packet, simtime_t startTime) const
{
    simtime_t duration = packet->getBitLength() / bitrate;
    simtime_t endTime = startTime + duration;
    IMobility *mobility = radio->getAntenna()->getMobility();
    Coord transmissionStartPosition = mobility->getPosition(startTime);
    Coord transmissionEndPosition = mobility->getPosition(endTime);
    return new IdealRadioSignalTransmission(radio, startTime, endTime, transmissionStartPosition, transmissionEndPosition, maximumCommunicationRange, maximumInterferenceRange);
}
