//
// Copyright (C) 2013 Andras Varga
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

#ifndef INET_INETWORKPROTOCOLCONTROLINFO_H_
#define INET_INETWORKPROTOCOLCONTROLINFO_H_

#include "Address.h"
#include "INetworkDatagram.h"


/**
 * Abstract interface for network-layer (L3) control info classes. This control info
 * is used for sending packets over a network protocol from higher layers,
 * and by the network protocol to report properties of a received datagram
 * to higher layers.
 */
class INET_API INetworkProtocolControlInfo
{
  public:
    virtual ~INetworkProtocolControlInfo() { }

    /**
     * Identifies the encapsulated protocol, using IANA IPv4/IPv6 Protocol Numbers.
     * If the network layer protocol uses a different way of id
     */
    virtual short getProtocol() const = 0;
    virtual void setProtocol(short protocol) = 0;

    /**
     * Source address of the datagram.
     */
    virtual Address getSourceAddress() const = 0;
    virtual void setSourceAddress(const Address & address) = 0;

    /**
     * Destination address of the datagram.
     */
    virtual Address getDestinationAddress() const = 0;
    virtual void setDestinationAddress(const Address & address) = 0;

    /**
     * Identifies the interface on which the datagram was received, or
     * should be sent (see InterfaceTable).
     */
    virtual int getInterfaceId() const = 0;
    virtual void setInterfaceId(int interfaceId) = 0;

    /**
     * Hop limit for a datagram to be sent, or remaining hops of
     * a datagram received. Maps to the IPv4 TTL and IPv6 hopLimit
     * fields. Ignored by network protocols that don't use hop count.
     */
    virtual short getHopLimit() const = 0;
    virtual void setHopLimit(short hopLimit) = 0;

    /**
     * If the destination address is multicast and the output interface
     * is also part of that multicast group, this flag determines whether
     * local applications should receive the datagram (loop=true) or not
     * (loop=false). Undefined when receiving a datagram.
     */
    virtual bool getMulticastLoop() const = 0;
    virtual void setMulticastLoop(bool multicastLoop) = 0;

//    virtual unsigned char getTrafficClass() const = 0;
//    virtual void setTrafficClass(unsigned char trafficClass) = 0;
};

#endif /* INETWORKPROTOCOLCONTROLINFO_H_ */
