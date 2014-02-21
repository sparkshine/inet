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

#include <CachedRadioChannel.h>

void CachedRadioChannel::finish()
{
    double cacheHitPercentage = 100 * (double)cacheHitCount / (double)cacheGetCount;
    EV_INFO << "Radio decision cache hit: " << cacheHitPercentage;
    recordScalar("Radio decision cache hit", cacheHitPercentage, "%");
}

const IRadioDecision *CachedRadioChannel::getCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission) const
{
    unsigned int transmissionId = transmission->getId();
    if (transmissionId >= cachedDecisions.size())
        return NULL;
    else
    {
        const std::vector<const IRadioDecision *> &cachedTransmissionDecisions = cachedDecisions[transmissionId - baseTransmissionId];
        unsigned int radioId = radio->getId();
        return radioId >= cachedTransmissionDecisions.size() ? NULL : cachedTransmissionDecisions[radioId];
    }
}

void CachedRadioChannel::setCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission, const IRadioDecision *decision)
{
    unsigned int transmissionId = transmission->getId();
    if (transmissionId >= cachedDecisions.size())
        cachedDecisions.resize(transmissionId + 1 - baseTransmissionId);
    std::vector<const IRadioDecision *> &cachedTransmissionDecisions = cachedDecisions[transmissionId - baseTransmissionId];
    unsigned int radioId = radio->getId();
    if (radioId >= cachedTransmissionDecisions.size())
        cachedTransmissionDecisions.resize(radioId + 1);
    cachedTransmissionDecisions[radioId] = decision;
}

void CachedRadioChannel::removeCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission)
{
    std::vector<const IRadioDecision *> &cachedTransmissionDecisions = cachedDecisions[transmission->getId() - baseTransmissionId];
    cachedTransmissionDecisions[radio->getId()] = NULL;
}

void CachedRadioChannel::invalidateCachedDecisions(const IRadioSignalTransmission *transmission)
{
    for (std::vector<std::vector<const IRadioDecision *> >::iterator it = cachedDecisions.begin(); it != cachedDecisions.end(); it++)
    {
        std::vector<const IRadioDecision *> &cachedTransmissionDecisions = *it;
        for (std::vector<const IRadioDecision *>::iterator jt = cachedTransmissionDecisions.begin(); jt != cachedTransmissionDecisions.end(); jt++)
        {
            const IRadioDecision *decision = *jt;
            if (decision)
            {
                const IRadioSignalReception *reception = decision->getReception();
                // TODO: instead of dropping the result we could update the noise only
                if (isOverlappingTransmission(transmission, reception))
                    invalidateCachedDecision(decision);
            }
        }
    }
}

void CachedRadioChannel::invalidateCachedDecision(const IRadioDecision *decision)
{
    const IRadioSignalReception *reception = decision->getReception();
    const XIRadio *radio = reception->getRadio();
    const IRadioSignalTransmission *transmission = reception->getTransmission();
    std::vector<const IRadioDecision *> &cachedTransmissionDecisions = cachedDecisions[transmission->getId() - baseTransmissionId];
    cachedTransmissionDecisions[radio->getId()] = NULL;
}

const IRadioDecision *CachedRadioChannel::receiveSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission) const
{
    cacheGetCount++;
    const IRadioDecision *decision = getCachedDecision(radio, transmission);
    if (decision)
    {
        cacheHitCount++;
        return decision;
    }
    else
        return RadioChannel::receiveSignal(radio, transmission);
}
