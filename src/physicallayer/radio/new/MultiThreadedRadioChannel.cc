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

#include <MultiThreadedRadioChannel.h>

MultiThreadedRadioChannel::~MultiThreadedRadioChannel()
{
    terminateWorkers();
}

void MultiThreadedRadioChannel::initializeWorkers(int workerCount)
{
    isWorkersEnabled = true;
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&pendingJobsLock, NULL);
    pthread_mutex_init(&cachedDecisionsLock, &mutexattr);
    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    pthread_cond_init(&pendingJobsCondition, &condattr);
    pthread_condattr_destroy(&condattr);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (int i = 0; i < workerCount; i++)
    {
        pthread_t *worker = new pthread_t();
        pthread_create(worker, &attr, callWorkerMain, this);
        // TODO: non portable
//        cpu_set_t cpuset;
//        CPU_ZERO(&cpuset);
//        for (int j = 0; j < 6; j++)
//            CPU_SET(j, &cpuset);
//        pthread_setaffinity_np(*worker, sizeof(cpu_set_t), &cpuset);
        workers.push_back(worker);
    }
    pthread_attr_destroy(&attr);
}

void MultiThreadedRadioChannel::terminateWorkers()
{
    isWorkersEnabled = false;
    pthread_mutex_lock(&pendingJobsLock);
    pendingInvalidateCacheJobs.clear();
    while (!pendingComputeCacheJobs.empty())
        pendingComputeCacheJobs.pop();
    pthread_cond_broadcast(&pendingJobsCondition);
    pthread_mutex_unlock(&pendingJobsLock);
    for (std::vector<pthread_t *>::iterator it = workers.begin(); it != workers.end(); it++)
    {
        void *status;
        pthread_t *worker = *it;
        pthread_join(*worker, &status);
        delete worker;
    }
    pthread_cond_destroy(&pendingJobsCondition);
    pthread_mutex_destroy(&pendingJobsLock);
    pthread_mutex_destroy(&cachedDecisionsLock);
}

void *MultiThreadedRadioChannel::callWorkerMain(void *argument)
{
    return ((MultiThreadedRadioChannel *)argument)->workerMain(argument);
}

void *MultiThreadedRadioChannel::workerMain(void *argument)
{
    while (isWorkersEnabled)
    {
        pthread_mutex_lock(&pendingJobsLock);
        while (isWorkersEnabled && pendingInvalidateCacheJobs.empty() && pendingComputeCacheJobs.empty())
            pthread_cond_wait(&pendingJobsCondition, &pendingJobsLock);
        EV_DEBUG << "Worker " << pthread_self() << " is looking for jobs on CPU " << sched_getcpu() << endl;
        if (!pendingInvalidateCacheJobs.empty())
        {
            const InvalidateCacheJob invalidateCacheJob = pendingInvalidateCacheJobs.front();
            pendingInvalidateCacheJobs.pop_front();
            EV_DEBUG << "Worker " << pthread_self() << " is running invalidate cache " << &invalidateCacheJob << endl;
            pthread_mutex_unlock(&pendingJobsLock);
            // TODO: this is a race condition with the main thread when receiving a signal
            invalidateCachedDecisions(invalidateCacheJob.transmission);
            pthread_mutex_lock(&pendingJobsLock);
            pthread_cond_broadcast(&pendingJobsCondition);
            pthread_mutex_unlock(&pendingJobsLock);
        }
        else if (!pendingComputeCacheJobs.empty())
        {
            const ComputeCacheJob computeCacheJob = pendingComputeCacheJobs.top();
            pendingComputeCacheJobs.pop();
            EV_DEBUG << "Worker " << pthread_self() << " is computing reception at " << computeCacheJob.receptionStartTime << endl;
            std::vector<const IRadioSignalTransmission *> *transmissionsCopy = new std::vector<const IRadioSignalTransmission *>(transmissions);
            pthread_mutex_unlock(&pendingJobsLock);
            const IRadioDecision *decision = computeDecision(computeCacheJob.radio, computeCacheJob.transmission, transmissionsCopy);
            setCachedDecision(computeCacheJob.radio, computeCacheJob.transmission, decision);
        }
        else
            pthread_mutex_unlock(&pendingJobsLock);
    }
    return NULL;
}

const IRadioDecision *MultiThreadedRadioChannel::getCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission) const
{
    const IRadioDecision *decision = NULL;
    pthread_mutex_lock(&cachedDecisionsLock);
    decision = CachedRadioChannel::getCachedDecision(radio, transmission);
    pthread_mutex_unlock(&cachedDecisionsLock);
    return decision;
}

void MultiThreadedRadioChannel::setCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission, const IRadioDecision *decision)
{
    pthread_mutex_lock(&cachedDecisionsLock);
    CachedRadioChannel::setCachedDecision(radio, transmission, decision);
    pthread_mutex_unlock(&cachedDecisionsLock);
}

void MultiThreadedRadioChannel::removeCachedDecision(const XIRadio *radio, const IRadioSignalTransmission *transmission)
{
    pthread_mutex_lock(&pendingJobsLock);
    CachedRadioChannel::removeCachedDecision(radio, transmission);
    pthread_mutex_unlock(&pendingJobsLock);
}

void MultiThreadedRadioChannel::invalidateCachedDecisions(const IRadioSignalTransmission *transmission)
{
    pthread_mutex_lock(&cachedDecisionsLock);
    CachedRadioChannel::invalidateCachedDecisions(transmission);
    pthread_mutex_unlock(&cachedDecisionsLock);
}

void MultiThreadedRadioChannel::invalidateCachedDecision(const IRadioDecision *decision)
{
    pthread_mutex_lock(&cachedDecisionsLock);
    CachedRadioChannel::invalidateCachedDecision(decision);
    pthread_mutex_unlock(&cachedDecisionsLock);
    const IRadioSignalReception *reception = decision->getReception();
    const XIRadio *radio = reception->getRadio();
    const IRadioSignalTransmission *transmission = reception->getTransmission();
    simtime_t startTime = reception->getStartTime();
    pthread_mutex_lock(&pendingJobsLock);
    pendingComputeCacheJobs.push(ComputeCacheJob(radio, transmission, startTime));
    pthread_mutex_unlock(&pendingJobsLock);
}

void MultiThreadedRadioChannel::transmitSignal(const XIRadio *transmitterRadio, const IRadioSignalTransmission *transmission)
{
    EV_DEBUG << "Radio " << transmitterRadio << " transmits signal " << transmission << endl;
    pthread_mutex_lock(&pendingJobsLock);
    RadioChannel::transmitSignal(transmitterRadio, transmission);
    pendingInvalidateCacheJobs.push_back(InvalidateCacheJob(transmission));
    for (std::vector<const XIRadio *>::iterator it = radios.begin(); it != radios.end(); it++) {
        const XIRadio *receiverRadio = *it;
        // TODO: merge with sendRadioFrame!
        if (transmitterRadio != receiverRadio && isPotentialReceiver(receiverRadio, transmission)) {
            simtime_t receptionStartTime = computeTransmissionStartArrivalTime(transmission, receiverRadio->getAntenna()->getMobility());
            pendingComputeCacheJobs.push(ComputeCacheJob(receiverRadio, transmission, receptionStartTime));
        }
    }
    // TODO: what shall we do with already running computation jobs?
    EV_DEBUG << "Transmission count: " << transmissions.size() << " pending job count: " << pendingComputeCacheJobs.size() << " cache hit count: " << cacheHitCount << " cache get count: " << cacheGetCount << " cache %: " << (100 * (double)cacheHitCount / (double)cacheGetCount) << "%\n";
    pthread_cond_broadcast(&pendingJobsCondition);
    pthread_mutex_unlock(&pendingJobsLock);
}

const IRadioDecision *MultiThreadedRadioChannel::receiveSignal(const XIRadio *radio, const IRadioSignalTransmission *transmission) const
{
    EV_DEBUG << "Radio " << radio << " receives signal " << transmission << endl;
    pthread_mutex_lock(&pendingJobsLock);
    while (!pendingInvalidateCacheJobs.empty())
        pthread_cond_wait(&pendingJobsCondition, &pendingJobsLock);
    pthread_mutex_unlock(&pendingJobsLock);
    return CachedRadioChannel::receiveSignal(radio, transmission);
}
