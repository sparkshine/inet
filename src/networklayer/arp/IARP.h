/*
 * Copyright (C) 2004 Andras Varga
 * Copyright (C) 2014 OpenSim Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __INET_IARPCACHE_H
#define __INET_IARPCACHE_H


#include "INETDefs.h"

#include "Address.h"
#include "IPv4Address.h"
#include "MACAddress.h"
#include "ModuleAccess.h"

class InterfaceEntry;

/**
 * Represents an IPv4 ARP module.
 */
class INET_API IARP
{
  public:
    /**
     * Sent in ARP cache change notification signals
     */
    class Notification : public cObject
    {
      public:
        Address l3Address;
        MACAddress macAddress;
        const InterfaceEntry *ie;
      public:
        Notification(Address l3Address, MACAddress macAddress, const InterfaceEntry *ie)
                : l3Address(l3Address), macAddress(macAddress), ie(ie) {}
    };

    // zoli:
    /** @brief Signals used to publish ARP state changes. */
    static const simsignal_t initiatedARPResolutionSignal;
    static const simsignal_t completedARPResolutionSignal;
    static const simsignal_t failedARPResolutionSignal;

    // levy:
    /** @brief A signal used to publish ARP state changes. */
    static const simsignal_t arpStateChangedSignal;

  public:
    virtual ~IARP() {}

    /**
     * Returns the IPv4 address for the given MAC address. If it is not available
     * (not in the cache, pending resolution, or already expired), UNSPECIFIED_ADDRESS
     * is returned.
     */
    virtual Address getL3AddressFor(const MACAddress&) const = 0;

    /**
     * Tries to resolve the given network address to a MAC address. If the MAC
     * address is not yet resolved it returns an unspecified address and starts
     * an address resolution procedure. A signal is emitted when the address
     * resolution procedure terminates.
     */
    virtual MACAddress resolveMACAddress(const Address& address, const InterfaceEntry *ie) = 0;
};

class INET_API IARPAccess : public ModuleAccess<IARP>
{
  public:
    IARPAccess() : ModuleAccess<IARP>("arp") {}
};

#endif
