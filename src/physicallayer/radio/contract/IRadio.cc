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

#include "IRadio.h"

simsignal_t IRadio::radioModeChangedSignal = cComponent::registerSignal("radioModeChanged");
simsignal_t IRadio::receptionStateChangedSignal = cComponent::registerSignal("receptionStateChanged");
simsignal_t IRadio::transmissionStateChangedSignal = cComponent::registerSignal("transmissionStateChanged");
simsignal_t IRadio::radioChannelChangedSignal = cComponent::registerSignal("radioChannelChanged");

cEnum *IRadio::radioModeEnum = NULL;
cEnum *IRadio::receptionStateEnum = NULL;
cEnum *IRadio::transmissionStateEnum = NULL;

Register_Enum(RadioMode,
              (IRadio::RADIO_MODE_OFF,
               IRadio::RADIO_MODE_SLEEP,
               IRadio::RADIO_MODE_RECEIVER,
               IRadio::RADIO_MODE_TRANSMITTER,
               IRadio::RADIO_MODE_TRANSCEIVER,
               IRadio::RADIO_MODE_SWITCHING));

Register_Enum(ReceptionState,
              (IRadio::RECEPTION_STATE_UNDEFINED,
               IRadio::RECEPTION_STATE_IDLE,
               IRadio::RECEPTION_STATE_BUSY,
               IRadio::RECEPTION_STATE_SYNCHRONIZING,
               IRadio::RECEPTION_STATE_RECEIVING));

Register_Enum(TransmissionState,
              (IRadio::TRANSMISSION_STATE_UNDEFINED,
               IRadio::TRANSMISSION_STATE_IDLE,
               IRadio::TRANSMISSION_STATE_TRANSMITTING));

const char *IRadio::getRadioModeName(RadioMode radioMode)
{
    if (!radioModeEnum)
        radioModeEnum = cEnum::get("RadioMode");
    return radioModeEnum->getStringFor(radioMode) + 11;
}

const char *IRadio::getRadioReceptionStateName(ReceptionState receptionState)
{
    if (!receptionStateEnum)
        receptionStateEnum = cEnum::get("RadioReceptionState");
    return receptionStateEnum->getStringFor(receptionState) + 22;
}

const char *IRadio::getRadioTransmissionStateName(TransmissionState transmissionState)
{
    if (!transmissionStateEnum)
        transmissionStateEnum = cEnum::get("RadioTransmissionState");
    return transmissionStateEnum->getStringFor(transmissionState) + 25;
}
