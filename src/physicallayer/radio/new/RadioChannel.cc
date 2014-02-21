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

#include <Radio.h>
#include <RadioChannel.h>

RadioChannel::~RadioChannel()
{
    delete backgroundNoise;
    delete attenuation;
}

simtime_t RadioChannel::computeArrivalTime(simtime_t time, Coord position, IMobility *mobility) const
{
    double distance;
    switch (mobilityApproximationCount)
    {
        case 0:
        {
            distance = position.distance(mobility->getCurrentPosition());
            break;
        }
        case 1:
            distance = position.distance(mobility->getPosition(time));
            break;
        case 2:
            // NOTE: repeat once again to approximate the movement during propagation
            distance = position.distance(mobility->getPosition(time));
            simtime_t propagationTime = distance / propagationSpeed;
            distance = position.distance(mobility->getPosition(time + propagationTime));
            break;
    }
    simtime_t propagationTime = distance / propagationSpeed;
    return time + propagationTime;
}

simtime_t RadioChannel::computeTransmissionStartArrivalTime(const IRadioSignalTransmission *transmission, IMobility *mobility) const
{
    return computeArrivalTime(transmission->getStartTime(), transmission->getStartPosition(), mobility);
}

simtime_t RadioChannel::computeTransmissionEndArrivalTime(const IRadioSignalTransmission *transmission, IMobility *mobility) const
{
    return computeArrivalTime(transmission->getEndTime(), transmission->getEndPosition(), mobility);
}

bool RadioChannel::isOverlappingTransmission(const IRadioSignalTransmission *transmission, const IRadioSignalReception *reception) const
{
    double propagationSpeed = getPropagationSpeed();
    simtime_t propagationStartTime = transmission->getStartPosition().distance(reception->getStartPosition()) / propagationSpeed;
    simtime_t propagationEndTime = transmission->getEndPosition().distance(reception->getEndPosition()) / propagationSpeed;
    simtime_t arrivalStartTime = transmission->getStartTime() + propagationStartTime;
    simtime_t arrivalEndTime = transmission->getEndTime() + propagationEndTime;
    return !(arrivalEndTime < reception->getStartTime() || arrivalStartTime > reception->getEndTime());
}

void RadioChannel::eraseAllExpiredTransmissions()
{
    // TODO: consider interfering with other not yet received signals (use maximum signal duration?)
    double xMinimum = DBL_MAX;
    double xMaximum = DBL_MIN;
    double yMinimum = DBL_MAX;
    double yMaximum = DBL_MIN;
    for (std::vector<const XIRadio *>::const_iterator it = radios.begin(); it != radios.end(); it++)
    {
        const XIRadio *radio = *it;
        IMobility *mobility = radio->getAntenna()->getMobility();
        Coord position = mobility->getCurrentPosition();
        if (position.x < xMinimum)
            xMinimum = position.x;
        if (position.x > xMaximum)
            xMaximum = position.x;
        if (position.y < yMinimum)
            yMinimum = position.y;
        if (position.y > yMaximum)
            yMaximum = position.y;
    }
    double distanceMaximum = Coord(xMinimum, yMinimum).distance(Coord(xMaximum, yMaximum));
    double propagationTimeMaximum = distanceMaximum / getPropagationSpeed();
    simtime_t transmissionEndTimeMinimum = simTime() - propagationTimeMaximum;
    for (std::vector<const IRadioSignalTransmission *>::iterator it = transmissions.begin(); it != transmissions.end();)
    {
        const IRadioSignalTransmission *transmission = *it;
        if (transmission->getEndTime() < transmissionEndTimeMinimum) {
            transmissions.erase(it);
        }
        else
            it++;
    }
}

const std::vector<const IRadioSignalTransmission *> *RadioChannel::computeOverlappingTransmissions(const IRadioSignalReception *reception, const std::vector<const IRadioSignalTransmission *> *transmissions) const
{
    std::vector<const IRadioSignalTransmission *> *overlappingTransmissions = new std::vector<const IRadioSignalTransmission *>();
    for (std::vector<const IRadioSignalTransmission *>::const_iterator it = transmissions->begin(); it != transmissions->end(); it++)
    {
        const IRadioSignalTransmission *candidateTransmission = *it;
        if (isOverlappingTransmission(candidateTransmission, reception))
            overlappingTransmissions->push_back(candidateTransmission);
    }
    return overlappingTransmissions;
}

const std::vector<const IRadioSignalReception *> *RadioChannel::computeOverlappingReceptions(const IRadioSignalReception *reception, const std::vector<const IRadioSignalTransmission *> *transmissions) const
{
    const XIRadio *radio = reception->getRadio();
    const IRadioSignalTransmission *transmission = reception->getTransmission();
    std::vector<const IRadioSignalReception *> *overlappingReceptions = new std::vector<const IRadioSignalReception *>();
    const std::vector<const IRadioSignalTransmission *> *overlappingTransmissions = computeOverlappingTransmissions(reception, transmissions);
    for (std::vector<const IRadioSignalTransmission *>::const_iterator it = overlappingTransmissions->begin(); it != overlappingTransmissions->end(); it++)
    {
        const IRadioSignalTransmission *overlappingTransmission = *it;
        if (overlappingTransmission != transmission)
            overlappingReceptions->push_back(attenuation->computeReception(radio, overlappingTransmission));
    }
    return overlappingReceptions;
}

const IRadioDecision *RadioChannel::computeDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission, const std::vector<const IRadioSignalTransmission *> *transmissions) const
{
    const IRadioSignalReception *reception = attenuation->computeReception(radio, transmission);
    const std::vector<const IRadioSignalReception *> *overlappingReceptions = computeOverlappingReceptions(reception, transmissions);
    const IRadioSignalNoise *noise = backgroundNoise ? backgroundNoise->computeNoise(reception) : NULL;
    return radio->getDecider()->computeDecision(reception, overlappingReceptions, noise);
}

void RadioChannel::transmitSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission)
{
    transmissions.push_back(transmission);
    eraseAllExpiredTransmissions();
}

const IRadioDecision *RadioChannel::receiveSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission) const
{
    return computeDecision(radio, transmission, const_cast<const std::vector<const IRadioSignalTransmission *> *>(&transmissions));
}

bool RadioChannel::isPotentialReceiver(const XIRadio *radio, const IRadioSignalTransmission *transmission) const
{
    if (maximumCommunicationRange == -1)
        return true;
    else
    {
        const IRadioAntenna *antenna = radio->getAntenna();
        IMobility *mobility = antenna->getMobility();
        simtime_t receptionStartTime = computeTransmissionStartArrivalTime(transmission, mobility);
        simtime_t receptionEndTime = computeTransmissionEndArrivalTime(transmission, mobility);
        Coord receptionStartPosition = mobility->getPosition(receptionStartTime);
        Coord receptionEndPosition = mobility->getPosition(receptionEndTime);
        double maxiumCommunicationRange = maximumCommunicationRange; // TODO: ???->computeMaximumCommunicationRange();
        return transmission->getStartPosition().distance(receptionStartPosition) < maxiumCommunicationRange ||
               transmission->getEndPosition().distance(receptionEndPosition) < maxiumCommunicationRange;
    }
}

void RadioChannel::sendRadioFrame(XIRadio *radio, XIRadioFrame *frame)
{
    XRadioFrame *radioFrame = check_and_cast<XRadioFrame *>(frame);
    const IRadioSignalTransmission *transmission = frame->getTransmission();
    for (std::vector<const XIRadio *>::const_iterator it = radios.begin(); it != radios.end(); it++)
    {
        const Radio *receiverRadio = check_and_cast<const Radio *>(*it);
        if (receiverRadio != radio && isPotentialReceiver(receiverRadio, transmission))
        {
            cGate *gate = const_cast<cGate *>(receiverRadio->RadioBase::getRadioGate()->getPathStartGate());
            simtime_t propagationTime = computeTransmissionStartArrivalTime(transmission, receiverRadio->getAntenna()->getMobility()) - simTime();
            XRadioFrame *frameCopy = new XRadioFrame(radioFrame->getTransmission());
            frameCopy->encapsulate(radioFrame->getEncapsulatedPacket()->dup());
            check_and_cast<cSimpleModule *>(radio)->sendDirect(frameCopy, propagationTime, radioFrame->getDuration(), gate);
        }
    }
}
