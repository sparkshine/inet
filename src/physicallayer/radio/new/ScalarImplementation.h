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

#ifndef __INET_SCALARIMPLEMENTATION_H_
#define __INET_SCALARIMPLEMENTATION_H_

#include "ImplementationBase.h"

class INET_API ScalarRadioSignalTransmission : public RadioSignalTransmissionBase
{
    protected:
        const double power;
        const double carrierFrequency;
        const double bandwidth;

    public:
        ScalarRadioSignalTransmission(const XIRadio *radio, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition, double power, double carrierFrequency, double bandwidth) :
            RadioSignalTransmissionBase(radio, startTime, endTime, startPosition, endPosition),
            power(power),
            carrierFrequency(carrierFrequency),
            bandwidth(bandwidth)
        {}

        virtual double getPower() const { return power; }
        virtual double getCarrierFrequency() const { return carrierFrequency; }
        virtual double getBandwidth() const { return bandwidth; }
};

class INET_API ScalarRadioSignalLoss : public IRadioSignalLoss
{
    protected:
        const double factor;

    public:
        ScalarRadioSignalLoss(double factor) :
            factor(factor)
        {}

        virtual double getFactor() const { return factor; }
};

class INET_API ScalarRadioSignalReception : public RadioSignalReceptionBase
{
    protected:
        const double power;
        const double carrierFrequency;
        const double bandwidth;

    public:
        ScalarRadioSignalReception(const XIRadio *radio, const IRadioSignalTransmission *transmission, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition, double power, double carrierFrequency, double bandwidth) :
            RadioSignalReceptionBase(radio, transmission, startTime, endTime, startPosition, endPosition),
            power(power),
            carrierFrequency(carrierFrequency),
            bandwidth(bandwidth)
        {}

        virtual double getPower() const { return power; }
        virtual double getCarrierFrequency() const { return carrierFrequency; }
        virtual double getBandwidth() const { return bandwidth; }
};

class INET_API ScalarRadioSignalNoise : public RadioSignalNoiseBase
{
    protected:
        const std::map<simtime_t, double> *powerChanges;
        // TODO: where's carrierFrequency and bandwidth

    public:
        ScalarRadioSignalNoise(simtime_t startTime, simtime_t endTime, const std::map<simtime_t, double> *powerChanges) :
            RadioSignalNoiseBase(startTime, endTime),
            powerChanges(powerChanges)
        {}

        virtual const std::map<simtime_t, double> *getPowerChanges() const { return powerChanges; }
};

class INET_API ScalarRadioSignalAttenuationBase : public virtual IRadioSignalAttenuation
{
    public:
        virtual const IRadioSignalReception *computeReception(const XIRadio *radio, const IRadioSignalTransmission *transmission) const;
};

class INET_API ScalarRadioSignalFreeSpaceAttenuation : public RadioSignalFreeSpaceAttenuationBase, public ScalarRadioSignalAttenuationBase
{
    public:
        ScalarRadioSignalFreeSpaceAttenuation(double alpha) :
            RadioSignalFreeSpaceAttenuationBase(alpha)
        {}

        virtual const IRadioSignalLoss *computeLoss(const XIRadio *radio, const IRadioSignalTransmission *transmission, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) const;
};

class INET_API ScalarRadioSignalCompoundAttenuation : public CompoundAttenuationBase, public ScalarRadioSignalAttenuationBase
{
    public:
        ScalarRadioSignalCompoundAttenuation(std::vector<const IRadioSignalAttenuation *> *elements) :
            CompoundAttenuationBase(elements)
        {}

        virtual const IRadioSignalLoss *computeLoss(const XIRadio *radio, const IRadioSignalTransmission *transmission, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) const;
};

class INET_API ScalarRadioBackgroundNoise : public IRadioBackgroundNoise
{
    protected:
        const double power;

    public:
        ScalarRadioBackgroundNoise(double power) :
            power(power)
        {}

    public:
        virtual double getPower() const { return power; }

        virtual const IRadioSignalNoise *computeNoise(const IRadioSignalReception *reception) const;
};

class INET_API ScalarRadioDecision : public RadioDecision
{
    protected:
        const double snrMinimum;
        // TODO: rssi, lqi, snr

    public:
        ScalarRadioDecision(const IRadioSignalReception *reception, bool isSuccessful, double snrMinimum) :
            RadioDecision(reception, isSuccessful),
            snrMinimum(snrMinimum)
        {}

        virtual double getSNRMinimum() const { return snrMinimum; }
};

class INET_API ScalarSNRRadioDecider : public SNRRadioDecider
{
    protected:
        virtual const IRadioSignalNoise *computeNoise(const std::vector<const IRadioSignalReception *> *receptions, const IRadioSignalNoise *backgroundNoise) const;
        virtual double computeSnr(const IRadioSignalReception *reception, const IRadioSignalNoise *noise) const;

    public:
        ScalarSNRRadioDecider(double snrThreshold) :
            SNRRadioDecider(snrThreshold)
        {}

        virtual const IRadioDecision *computeDecision(const IRadioSignalReception *candidateReception, const std::vector<const IRadioSignalReception *> *overlappingReceptions, const IRadioSignalNoise *backgroundNoise) const;
};

class INET_API ScalarRadioSignalTransmissionFactory : public IRadioSignalTransmissionFactory
{
    protected:
        double bitrate;
        // TODO: is it the preamble duration?
        double headerBitLength;
        double power;
        double carrierFrequency;
        double bandwidth;

    public:
        ScalarRadioSignalTransmissionFactory(double bitrate, double headerBitLength, double power, double carrierFrequency, double bandwidth) :
            bitrate(bitrate),
            headerBitLength(headerBitLength),
            power(power),
            carrierFrequency(carrierFrequency),
            bandwidth(bandwidth)
        {}

        virtual const IRadioSignalTransmission *createTransmission(const XIRadio *radio, const cPacket *packet, simtime_t startTime) const;
};

#endif
