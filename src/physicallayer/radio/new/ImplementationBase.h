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

#ifndef __INET_IMPLEMENTATIONBASE_H_
#define __INET_IMPLEMENTATIONBASE_H_

#include "IRadioSignalLoss.h"
#include "IRadioBackgroundNoise.h"
#include "IRadioSignalAttenuation.h"
#include "IRadioSignalTransmissionFactory.h"

// TODO: revise all names here and also in contract.h
// TODO: optimize interface in terms of constness, use of references, etc.
// TODO: add proper destructors with freeing resources
// TODO: add delete operator calls where appropriate and do proper memory management
// TODO: extend radio decider interface to allow a separate decision for the detection of preambles during synchronization
// TODO: extend radio decider interface to provide reception state for listeners? and support for carrier sensing for MACs
// TODO: avoid the need for subclassing radio and radio channel to be able to have only one parameterizable radio and radio channel NED types
// TODO: add classification of radios into grid cells to be able provide an approximation of the list of radios within communication range quickly
// TODO: extend attenuation model with obstacles, is it a separate model or just another attenuation model?
// TODO: add computation for maximum communication range, using computation for maximum transmission signal power and minimum reception power?
// TODO: refactor optimizing radio channel to allow turning on and off optimization via runtime parameters instead of subclassing
// TODO: extend interface to allow CUDA optimizations e.g. with adding Pi(x, y, z, t, f, b) and SNRi, etc. multiple nested loops to compute the minimum SNR for all transmissions at all receiver radios at once
// TODO: add a skeleton for sampled radio signals or maybe support for GNU radio?
// TODO: add NED modules to provide compound parameters for radio and radio channel
// TODO: how do we combine attenuation and antenna models, do we miss something?
// TODO: who is converting receptions to packets as opposed to signal producer?

class INET_API RadioSignalTransmissionBase : public IRadioSignalTransmission
{
    protected:
        static unsigned int nextId;

    protected:
        const unsigned int id;

        const XIRadio *radio;

        const simtime_t startTime;
        const simtime_t endTime;

        const Coord startPosition;
        const Coord endPosition;
        const Coord startAngularPosition;
        const Coord endAngularPosition;

        double propagationSpeed;

    public:
        RadioSignalTransmissionBase(const XIRadio *radio, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) :
            id(nextId++),
            radio(radio),
            startTime(startTime),
            endTime(endTime),
            startPosition(startPosition),
            endPosition(endPosition),
            propagationSpeed(SPEED_OF_LIGHT)
        {}

        virtual unsigned int getId() const { return id; }

        virtual simtime_t getStartTime() const { return startTime; }
        virtual simtime_t getEndTime() const { return endTime; }
        virtual simtime_t getDuration() const { return endTime - startTime; }

        virtual Coord getStartPosition() const { return startPosition; }
        virtual Coord getEndPosition() const { return endPosition; }

        virtual double getPropagationSpeed() const { return propagationSpeed; }

        virtual const XIRadio *getRadio() const { return radio; }
};

class INET_API RadioSignalReceptionBase : public IRadioSignalReception
{
    protected:
        const XIRadio *radio;
        const IRadioSignalTransmission *transmission;

        const simtime_t startTime;
        const simtime_t endTime;

        const Coord startPosition;
        const Coord endPosition;

    public:
        RadioSignalReceptionBase(const XIRadio *radio, const IRadioSignalTransmission *transmission, simtime_t startTime, simtime_t endTime, Coord startPosition, Coord endPosition) :
            radio(radio),
            transmission(transmission),
            startTime(startTime),
            endTime(endTime),
            startPosition(startPosition),
            endPosition(endPosition)
        {}

        virtual simtime_t getStartTime() const { return startTime; }
        virtual simtime_t getEndTime() const { return endTime; }
        virtual simtime_t getDuration() const { return endTime - startTime; }

        virtual Coord getStartPosition() const { return startPosition; }
        virtual Coord getEndPosition() const { return endPosition; }

        virtual const XIRadio *getRadio() const { return radio; }
        virtual const IRadioSignalTransmission *getTransmission() const { return transmission; }
};

class INET_API RadioSignalNoiseBase : public IRadioSignalNoise
{
    protected:
        const simtime_t startTime;
        const simtime_t endTime;

    public:
        RadioSignalNoiseBase(simtime_t startTime, simtime_t endTime) :
            startTime(startTime),
            endTime(endTime)
        {}

        virtual simtime_t getStartTime() const { return startTime; }
        virtual simtime_t getEndTime() const { return endTime; }
        virtual simtime_t getDuration() const { return endTime - startTime; }
};

class INET_API RadioAntennaBase : public IRadioAntenna
{
    protected:
        IMobility *mobility;

    public:
        RadioAntennaBase(IMobility *mobility) :
            mobility(mobility)
        {}

        virtual IMobility *getMobility() const { return mobility; }
};

class INET_API IsotropicRadioAntenna : public RadioAntennaBase
{
    public:
        IsotropicRadioAntenna(IMobility *mobility) :
            RadioAntennaBase(mobility)
        {}

        virtual double getGain(Coord direction) const { return 1; }
};

class INET_API DipoleRadioAntenna : public RadioAntennaBase
{
    protected:
        const double length;

    public:
        DipoleRadioAntenna(IMobility *mobility, double length) :
            RadioAntennaBase(mobility),
            length(length)
        {}

        virtual double getLength() const { return length; }
        virtual double getGain(Coord direction) const { return 1; }
};

class INET_API RadioSignalFreeSpaceAttenuationBase : public virtual IRadioSignalAttenuation
{
    protected:
        const double alpha;

    protected:
        virtual double computePathLoss(const IRadioSignalTransmission *transmission, simtime_t receptionStartTime, simtime_t receptionEndTime, Coord receptionStartPosition, Coord receptionEndPosition, double carrierFrequency) const;

    public:
        RadioSignalFreeSpaceAttenuationBase(double alpha) :
            alpha(alpha)
        {}

        virtual double getAlpha() const { return alpha; }
};

class INET_API CompoundAttenuationBase : public IRadioSignalAttenuation
{
    protected:
        std::vector<const IRadioSignalAttenuation *> *elements;

    public:
        CompoundAttenuationBase(std::vector<const IRadioSignalAttenuation *> *elements) :
            elements(elements)
        {}
};

class INET_API RadioDecision : public IRadioDecision, public cObject
{
    protected:
        const IRadioSignalReception *reception;
        const bool isSuccessfulFlag;

    public:
        RadioDecision(const IRadioSignalReception *reception, bool isSuccessful) :
            reception(reception),
            isSuccessfulFlag(isSuccessful)
        {}

        virtual const IRadioSignalReception *getReception() const { return reception; }
        virtual bool isSuccessful() const { return isSuccessfulFlag; }
};

class INET_API SNRRadioDecider : public IRadioDecider
{
    protected:
        const double snrThreshold;

    protected:
        virtual const IRadioSignalNoise *computeNoise(const std::vector<const IRadioSignalReception *> *receptions, const IRadioSignalNoise *backgroundNoise) const = 0;
        virtual double computeSnr(const IRadioSignalReception *reception, const IRadioSignalNoise *noise) const = 0;

    public:
        SNRRadioDecider(double snrThreshold) :
            snrThreshold(snrThreshold)
        {}

        virtual double getSNRThreshold() const { return snrThreshold; }
        virtual const IRadioDecision *computeDecision(const IRadioSignalReception *candidateReception, const std::vector<const IRadioSignalReception *> *overlappingReceptions, const IRadioSignalNoise *backgroundNoise) const;
};

#endif
