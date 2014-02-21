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

#ifndef __INET_MULTITHREADEDRADIOCHANNEL_H_
#define __INET_MULTITHREADEDRADIOCHANNEL_H_

#include <queue>
#include <deque>
#include <pthread.h>
#include "CachedRadioChannel.h"

// TODO: error handling to avoid leaving locks when unwinding
class INET_API MultiThreadedRadioChannel : public CachedRadioChannel
{
    protected:
        class InvalidateCacheJob
        {
            public:
                const IRadioSignalTransmission *transmission;

            public:
                InvalidateCacheJob(const IRadioSignalTransmission *transmission) :
                    transmission(transmission)
                {}
        };

        class ComputeCacheJob
        {
            public:
                const XIRadio *radio;
                const IRadioSignalTransmission *transmission;
                simtime_t receptionStartTime;

            public:
                ComputeCacheJob(const XIRadio *radio, const IRadioSignalTransmission *transmission, simtime_t receptionStartTime) :
                    radio(radio),
                    transmission(transmission),
                    receptionStartTime(receptionStartTime)
                {}

                static bool compareReceptionStartTimes(const ComputeCacheJob &job1, const ComputeCacheJob &job2)
                {
                    return job1.receptionStartTime < job2.receptionStartTime;
                }
        };

        class ComputeCacheJobComparer
        {
            public:
                bool operator()(const ComputeCacheJob &job1, const ComputeCacheJob &job2)
                {
                    return job1.receptionStartTime > job2.receptionStartTime;
                }
        };

    protected:
        bool isWorkersEnabled;
        std::vector<pthread_t *> workers;
        mutable pthread_mutex_t pendingJobsLock;
        mutable pthread_mutex_t cachedDecisionsLock;
        mutable pthread_cond_t pendingJobsCondition;
        std::deque<InvalidateCacheJob> pendingInvalidateCacheJobs;
        std::priority_queue<ComputeCacheJob, std::vector<ComputeCacheJob>, ComputeCacheJobComparer> pendingComputeCacheJobs;

    protected:
        virtual void initializeWorkers(int workerCount);
        virtual void terminateWorkers();
        virtual void *workerMain(void *argument);
        static void *callWorkerMain(void *argument);

        virtual const IRadioDecision *getCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission) const;
        virtual void setCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission, const IRadioDecision *decision);
        virtual void removeCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission);
        virtual void invalidateCachedDecisions(const IRadioSignalTransmission *transmission);
        virtual void invalidateCachedDecision(const IRadioDecision *decision);

    public:
        MultiThreadedRadioChannel() :
            CachedRadioChannel()
        {}

        MultiThreadedRadioChannel(const IRadioBackgroundNoise *noise, const IRadioSignalAttenuation *attenuation, int workerCount) :
            CachedRadioChannel(noise, attenuation)
        { initializeWorkers(workerCount); }

        virtual ~MultiThreadedRadioChannel();

        virtual void transmitSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission);
        virtual const IRadioDecision *receiveSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission) const;
};

#endif
