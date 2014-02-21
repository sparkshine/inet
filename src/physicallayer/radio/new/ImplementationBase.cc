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

#include "ImplementationBase.h"

unsigned int RadioSignalTransmissionBase::nextId = 0;

double RadioSignalFreeSpaceAttenuationBase::computePathLoss(const IRadioSignalTransmission *transmission, simtime_t receptionStartTime, simtime_t receptionEndTime, Coord receptionStartPosition, Coord receptionEndPosition, double carrierFrequency) const
{
    /** @brief
     *       waveLength ^ 2
     *   -----------------------
     *   16 * pi ^ 2 * d ^ alpha
     */
//    TODO: add parameter to use more precise distance approximation?
//    double startDistance = transmission->getStartPosition().distance(receptionStartPosition);
//    double endDistance = transmission->getEndPosition().distance(receptionEndPosition);
//    double distance = (startDistance + endDistance) / 2;
    double distance = transmission->getStartPosition().distance(receptionStartPosition);
    double waveLength = transmission->getPropagationSpeed() / carrierFrequency;
    // NOTE: this check allows to get the same result from the GPU and the CPU when the alpha is exactly 2
    double raisedDistance = alpha == 2.0 ? distance * distance : pow(distance, alpha);
    return distance == 0.0 ? 1.0 : waveLength * waveLength / (16.0 * M_PI * M_PI * raisedDistance);
}

const IRadioDecision *SNRRadioDecider::computeDecision(const IRadioSignalReception *candidateReception, const std::vector<const IRadioSignalReception *> *overlappingReceptions, const IRadioSignalNoise *backgroundNoise) const
{
    const IRadioSignalNoise *noise = computeNoise(overlappingReceptions, backgroundNoise);
    double snrMinimum = computeSnr(candidateReception, noise);
    return new RadioDecision(candidateReception, snrMinimum > snrThreshold);
}
