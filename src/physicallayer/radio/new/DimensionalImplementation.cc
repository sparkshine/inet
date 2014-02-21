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

#include "DimensionalImplementation.h"
#include "IRadioChannel.h"

const IRadioSignalReception *DimensionalRadioSignalAttenuationBase::computeReception(const XIRadio *receiverRadio, const IRadioSignalTransmission *transmission) const
{
    const XIRadioChannel *radioChannel = receiverRadio->getXRadioChannel();
    const XIRadio *transmitterRadio = transmission->getRadio();
    const IRadioAntenna *receiverAntenna = receiverRadio->getAntenna();
    const IRadioAntenna *transmitterAntenna = transmitterRadio->getAntenna();
    const DimensionalRadioSignalTransmission *dimensionalTransmission = check_and_cast<const DimensionalRadioSignalTransmission *>(transmission);
    IMobility *receiverAntennaMobility = receiverAntenna->getMobility();
    simtime_t receptionStartTime = radioChannel->computeTransmissionStartArrivalTime(transmission, receiverAntennaMobility);
    simtime_t receptionEndTime = radioChannel->computeTransmissionEndArrivalTime(transmission, receiverAntennaMobility);
    Coord receptionStartPosition = receiverAntennaMobility->getPosition(receptionStartTime);
    Coord receptionEndPosition = receiverAntennaMobility->getPosition(receptionEndTime);
    Coord direction = receptionStartPosition - transmission->getStartPosition();
    // TODO: use antenna gains
    double transmitterAntennaGain = transmitterAntenna->getGain(direction);
    double receiverAntennaGain = receiverAntenna->getGain(direction);
    const ConstMapping *attenuationFactor = check_and_cast<const DimensionalRadioSignalLoss *>(computeLoss(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition))->getFactor();
    const Mapping *transmissionPower = dimensionalTransmission->getPower();
    const Mapping *receptionPower = MappingUtils::multiply(*transmissionPower, *attenuationFactor, Argument::MappedZero);
    return new DimensionalRadioSignalReception(receiverRadio, transmission, receptionStartTime, receptionEndTime, receptionStartPosition, receptionEndPosition, receptionPower, dimensionalTransmission->getCarrierFrequency());
}

const IRadioSignalLoss *DimensionalRadioSignalFreeSpaceAttenuation::computeLoss(const XIRadio *radio, const IRadioSignalTransmission *transmission, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) const
{
    const DimensionalRadioSignalTransmission *dimensionalTransmission = check_and_cast<const DimensionalRadioSignalTransmission *>(transmission);
    // TODO: iterate over frequencies in power mapping
    LossConstMapping *pathLoss = new LossConstMapping(DimensionSet::timeFreqDomain, computePathLoss(transmission, startTime, endTime, startPosition, endPosition, dimensionalTransmission->getCarrierFrequency()));
    return new DimensionalRadioSignalLoss(pathLoss);
}

const IRadioSignalNoise *DimensionalRadioBackgroundNoise::computeNoise(const IRadioSignalReception *reception) const
{
    // TODO:
    throw cRuntimeError("Not yet implemented");
}

const IRadioSignalNoise *DimensionalSNRRadioDecider::computeNoise(const std::vector<const IRadioSignalReception *> *receptions, const IRadioSignalNoise *backgroundNoise) const
{
    // TODO:
    throw cRuntimeError("Not yet implemented");
}

double DimensionalSNRRadioDecider::computeSnr(const IRadioSignalReception *reception, const IRadioSignalNoise *noise) const
{
    // TODO:
    throw cRuntimeError("Not yet implemented");
}

const IRadioSignalTransmission *DimensionalRadioSignalTransmissionFactory::createTransmission(const XIRadio *radio, const cPacket *packet, simtime_t startTime) const
{
    simtime_t duration = packet->getBitLength() / bitrate;
    simtime_t endTime = startTime + duration;
    IMobility *mobility = radio->getAntenna()->getMobility();
    Coord transmissionStartPosition = mobility->getPosition(startTime);
    Coord transmissionEndPosition = mobility->getPosition(endTime);
    Mapping *powerMapping = MappingUtils::createMapping(Argument::MappedZero, DimensionSet::timeFreqDomain, Mapping::LINEAR);
    Argument position(DimensionSet::timeFreqDomain);
    position.setArgValue(Dimension::frequency, carrierFrequency - bandwidth / 2);
    position.setTime(startTime);
    powerMapping->setValue(position, power);
    position.setTime(endTime);
    powerMapping->setValue(position, power);
    position.setArgValue(Dimension::frequency, carrierFrequency + bandwidth / 2);
    position.setTime(startTime);
    powerMapping->setValue(position, power);
    position.setTime(endTime);
    powerMapping->setValue(position, power);
    return new DimensionalRadioSignalTransmission(radio, startTime, endTime, transmissionStartPosition, transmissionEndPosition, powerMapping, carrierFrequency);
}
