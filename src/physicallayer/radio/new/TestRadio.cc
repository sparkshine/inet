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

#include <iostream>
#include "TestRadio.h"
#include "StationaryMobility.h"
#include "Radio.h"
#include "RadioChannel.h"
#include "MultiThreadedRadioChannel.h"
#include "CUDARadioChannel.h"
#include "IdealImplementation.h"
#include "ScalarImplementation.h"
#include "DimensionalImplementation.h"

#define debug true

#define TIME(CODE) { struct timespec timeStart, timeEnd; clock_gettime(CLOCK_REALTIME, &timeStart); CODE; clock_gettime(CLOCK_REALTIME, &timeEnd); \
    std::cout << "Elapsed time is " << ((timeEnd.tv_sec - timeStart.tv_sec) + (timeEnd.tv_nsec - timeStart.tv_nsec) / 1E+9) << "s during running " << #CODE << endl << endl; }

double random(double a, double b)
{
    double r = (double)rand() / RAND_MAX;
    return a + r * (b-a);
}

void testIdealRadio()
{
    cPacket *senderMacFrame = new cPacket("Hello", 0, 64 * 8);

    IRadioSignalAttenuation *attenuation = new IdealRadioSignalFreeSpaceAttenuation();
    XIRadioChannel *channel = new RadioChannel(NULL, attenuation);

    IMobility *senderMobility = new StationaryMobility(Coord(0, 0));
    IRadioAntenna *senderAntenna = new IsotropicRadioAntenna(senderMobility);
    IRadioDecider *senderDecider = new IdealRadioDecider(false);
    IRadioSignalTransmissionFactory *senderTransmissionFactory = new IdealRadioSignalTransmissionFactory(2E+6, 100, 500);
    XIRadio *senderRadio = new Radio(IRadio::RADIO_MODE_TRANSMITTER, senderTransmissionFactory, senderAntenna, senderDecider, channel);

    IMobility *receiverMobility = new StationaryMobility(Coord(100, 0));
    IRadioAntenna *receiverAntenna = new IsotropicRadioAntenna(receiverMobility);
    IRadioDecider *receiverDecider = new IdealRadioDecider(false);
    XIRadio *receiverRadio = new Radio(IRadio::RADIO_MODE_RECEIVER, NULL, receiverAntenna, receiverDecider, channel);

    XIRadioFrame *radioFrame = senderRadio->transmitPacket(senderMacFrame, simTime());
    cPacket *receiverMacFrame = receiverRadio->receivePacket(radioFrame);

    if (debug)
        std::cout << "IdealRadio: " << receiverMacFrame->getName() << " : " << receiverMacFrame->getKind() << endl;

    delete channel;
}

void testScalarRadio()
{
    cPacket *senderMacFrame = new cPacket("Hello", 0, 64 * 8);

    IRadioBackgroundNoise *backgroundNoise = new ScalarRadioBackgroundNoise(1E-12);
    IRadioSignalAttenuation *attenuation = new ScalarRadioSignalFreeSpaceAttenuation(2);
    XIRadioChannel *channel = new RadioChannel(backgroundNoise, attenuation);

    IMobility *senderMobility = new StationaryMobility(Coord(0, 0));
    IRadioAntenna *senderAntenna = new IsotropicRadioAntenna(senderMobility);
    IRadioDecider *senderDecider = new ScalarSNRRadioDecider(10);
    IRadioSignalTransmissionFactory *senderTransmissionFactory = new ScalarRadioSignalTransmissionFactory(2E+6, 100, 1E-3, 2.4E+9, 1E+6);
    XIRadio *senderRadio = new Radio(IRadio::RADIO_MODE_TRANSMITTER, senderTransmissionFactory, senderAntenna, senderDecider, channel);

    IMobility *receiverMobility = new StationaryMobility(Coord(100, 0));
    IRadioAntenna *receiverAntenna = new IsotropicRadioAntenna(receiverMobility);
    IRadioDecider *receiverDecider = new ScalarSNRRadioDecider(10);
    XIRadio *receiverRadio = new Radio(IRadio::RADIO_MODE_RECEIVER, NULL, receiverAntenna, receiverDecider, channel);

    XIRadioFrame *radioFrame = senderRadio->transmitPacket(senderMacFrame, simTime());
    cPacket *receiverMacFrame = receiverRadio->receivePacket(radioFrame);

    if (debug)
        std::cout << "ScalarRadio: " << receiverMacFrame->getName() << " : " << receiverMacFrame->getKind() << endl;
}

void testDimensionalRadio()
{
    cPacket *senderMacFrame = new cPacket("Hello", 0, 64 * 8);

    IRadioBackgroundNoise *backgroundNoise = new DimensionalRadioBackgroundNoise(1E-12);
    IRadioSignalAttenuation *attenuation = new DimensionalRadioSignalFreeSpaceAttenuation(2);
    XIRadioChannel *channel = new RadioChannel(backgroundNoise, attenuation);

    IMobility *senderMobility = new StationaryMobility(Coord(0, 0));
    IRadioAntenna *senderAntenna = new IsotropicRadioAntenna(senderMobility);
    IRadioDecider *senderDecider = new DimensionalSNRRadioDecider(10);
    IRadioSignalTransmissionFactory *senderTransmissionFactory = new DimensionalRadioSignalTransmissionFactory(2E+6, 1E-3, 2.4E+9, 1E+6);
    XIRadio *senderRadio = new Radio(IRadio::RADIO_MODE_TRANSMITTER, senderTransmissionFactory, senderAntenna, senderDecider, channel);

    IMobility *receiverMobility = new StationaryMobility(Coord(100, 0));
    IRadioAntenna *receiverAntenna = new IsotropicRadioAntenna(receiverMobility);
    IRadioDecider *receiverDecider = new DimensionalSNRRadioDecider(10);
    XIRadio *receiverRadio = new Radio(IRadio::RADIO_MODE_RECEIVER, NULL, receiverAntenna, receiverDecider, channel);

    XIRadioFrame *radioFrame = senderRadio->transmitPacket(senderMacFrame, simTime());
    cPacket *receiverMacFrame = receiverRadio->receivePacket(radioFrame);

    if (debug)
        std::cout << "DimensionalRadio: " << receiverMacFrame->getName() << " : " << receiverMacFrame->getKind() << endl;

    delete channel;
}

void testMultipleScalarRadios(XIRadioChannel *channel, int radioCount, int frameCount, simtime_t duration, double playgroundSize)
{
    int successfulReceptionCount = 0;
    int failedReceptionCount = 0;
    std::vector<XIRadio *> radios;
    std::vector<XIRadioFrame *> radioFrames;
    std::cout << "Radio count: " << radioCount << ", frame count: " << frameCount << ", duration: " << duration << ", playground size: " << playgroundSize << endl;

    srand(0);
    for (int i = 0; i < radioCount; i++)
    {
        IMobility *mobility = new StationaryMobility(Coord(random(0, playgroundSize), random(0, playgroundSize)));
        IRadioAntenna *antenna = new IsotropicRadioAntenna(mobility);
        IRadioDecider *decider = new ScalarSNRRadioDecider(10);
        IRadio::RadioMode radioMode = random(0, 1) < 0.5 ? IRadio::RADIO_MODE_TRANSMITTER : IRadio::RADIO_MODE_RECEIVER;
        IRadioSignalTransmissionFactory *transmissionFactory = new ScalarRadioSignalTransmissionFactory(2E+6, 100, 1E-3, 2.4E+9, 1E+6);
        XIRadio *radio = new Radio(radioMode, transmissionFactory, antenna, decider, channel);
        radios.push_back(radio);
    }

    for (int i = 0; i < frameCount; i++)
    {
        int index = random(0, radioCount);
        XIRadio *radio = radios[index];
        simtime_t startTime = random(0, duration.dbl());
        cPacket *senderMacFrame = new cPacket("Hello", 0, 64 * 8);
        if (debug)
            std::cout << "Sending at " << startTime << " from " << index << endl;
        XIRadioFrame *radioFrame = radio->transmitPacket(senderMacFrame, startTime);
        radioFrames.push_back(radioFrame);
    }

    for (std::vector<XIRadio *>::iterator it = radios.begin(); it != radios.end(); it++)
    {
        XIRadio *radio = *it;
        for (std::vector<XIRadioFrame *>::iterator jt = radioFrames.begin(); jt != radioFrames.end(); jt++)
        {
            XIRadioFrame *radioFrame = *jt;
            cPacket *receiverMacFrame = radio->receivePacket(radioFrame);
            if (receiverMacFrame->getKind() == 1)
                successfulReceptionCount++;
            else
                failedReceptionCount++;
            if (debug)
            {
                ScalarRadioDecision *decision = check_and_cast<ScalarRadioDecision *>(receiverMacFrame->getControlInfo());
                const ScalarRadioSignalReception *reception = check_and_cast<const ScalarRadioSignalReception *>(decision->getReception());
                std::cout.precision(16);
                std::cout << "Radio decision arrival time: " << reception->getStartTime() << ", reception power: " << reception->getPower() << ", snr minimum: " << decision->getSNRMinimum() << endl;
            }
            receiverMacFrame->removeControlInfo();
            check_and_cast<cPacket *>(radioFrame)->encapsulate(receiverMacFrame);
        }
    }

    std::cout << "Successful reception count: " << successfulReceptionCount << ", failed reception count: " << failedReceptionCount << endl;
}

void testMultipleScalarRadiosWithAllChannels(int radioCount, int frameCount, simtime_t duration, double playgroundSize)
{
    IRadioBackgroundNoise *backgroundNoise = new ScalarRadioBackgroundNoise(1E-12);
    IRadioSignalAttenuation *attenuation = new ScalarRadioSignalFreeSpaceAttenuation(2);
    RadioChannel *radioChannel = new RadioChannel(backgroundNoise, attenuation);
    MultiThreadedRadioChannel *multiThreadedRadioChannel = new MultiThreadedRadioChannel(backgroundNoise, attenuation, 3);
    CUDARadioChannel *cudaRadioChannel = new CUDARadioChannel(backgroundNoise, attenuation);
    //TIME(testMultipleScalarRadios(radioChannel, radioCount, frameCount, duration, playgroundSize));
    //TIME(testMultipleScalarRadios(multiThreadedRadioChannel, radioCount, frameCount, duration, playgroundSize));
    TIME(testMultipleScalarRadios(cudaRadioChannel, radioCount, frameCount, duration, playgroundSize));
}

Define_Module(TestRadio);

void TestRadio::initialize(int stage)
{
//    testIdealRadio();
//    testScalarRadio();
//    testDimensionalRadio();
//    testMultipleScalarRadiosWithAllChannels(10, 100, 1, 10);
//    testMultipleScalarRadiosWithAllChannels(10, 100, 2, 100);
//    testMultipleScalarRadiosWithAllChannels(20, 200, 2, 100);
//    testMultipleScalarRadiosWithAllChannels(40, 400, 2, 100);
    testMultipleScalarRadiosWithAllChannels(80, 800, 2, 100);
    throw cTerminationException("Test finished");
}
