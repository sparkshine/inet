//
// Copyright (C) 2013 Brno University of Technology (http://nes.fit.vutbr.cz/ansa)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 3
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
// Authors: Veronika Rybova, Tomas Prochazka (mailto:xproch21@stud.fit.vutbr.cz), Jiri Trhlik (mailto:JiriTM@gmail.com)
//          Vladimir Vesely (mailto:ivesely@fit.vutbr.cz)

#ifndef __INET_PIMMULTICASTROUTE_H
#define __INET_PIMMULTICASTROUTE_H

#include "IPv4Route.h"

#include "PIMTimer_m.h"
#include "InterfaceEntry.h"
#include "PIMInterfaceTable.h" // for PIMMode

/**
 * PIM IPv4 multicast route in IRoutingTable.
 */
class INET_API PIMMulticastRoute : public IPv4MulticastRoute
{
    public:
        /** Route flags. Added to each route. */
        enum Flag
        {
            NO_FLAG = 0,
            D       = 0x01,              /**< Dense */
            S       = 0x02,              /**< Sparse */
            C       = 0x04,              /**< Connected */
            P       = 0x08,              /**< Pruned */
            A       = 0x10,              /**< Source is directly connected */
            F       = 0x20,              /**< Register flag*/
            T       = 0x40               /**< SPT bit*/
        };

        /** States of each outgoing interface. */
        enum InterfaceState
        {
            Forward,
            Pruned
        };

        /** Assert States of each outgoing interface. */
        enum AssertState
        {
            NoInfo = 0,
            Winner = 1,
            Loser = 2
        };

        /**  Register machine States. */
        enum RegisterState
        {
            NoInfoRS = 0,
            Join = 1,
            Prune = 2,
            JoinPending = 3
        };

        /**
         * @brief Structure of incoming interface.
         * @details E.g.: GigabitEthernet1/4, RPF nbr 10.10.51.145
         */
        struct PIMInInterface : public InInterface
        {
            IPv4Address nextHop;            /**< RF neighbor */

            PIMInInterface(InterfaceEntry *ie, int interfaceId, IPv4Address nextHop)
                : InInterface(ie), nextHop(nextHop) {}
            int getInterfaceId() const { return ie->getInterfaceId(); }
        };

        /**
         * @brief Structure of outgoing interface.
         * @details E.g.: Ethernet0, Forward/Sparse, 5:29:15/0:02:57
         */
        struct PIMOutInterface : public OutInterface
        {
            InterfaceState          forwarding;         /**< Forward or Pruned */
            PIMInterface::PIMMode   mode;               /**< Dense, Sparse, ... */
            cMessage                *pruneTimer;        /**< Pointer to PIM Prune Timer*/
            PIMet                   *expiryTimer;       /**< Pointer to PIM Expiry Timer*/
            AssertState             assert;             /**< Assert state. */
            RegisterState           regState;           /**< Register state. */
            bool                    shRegTun;           /**< Show interface which is also register tunnel interface*/

            PIMOutInterface(InterfaceEntry *ie)
                : OutInterface(ie, false) {}
            PIMOutInterface(InterfaceEntry *ie, InterfaceState forwarding, PIMInterface::PIMMode mode, cMessage *pruneTimer,
                    PIMet *expiryTimer, AssertState assert, RegisterState regState, bool show)
                : OutInterface(ie, false), forwarding(forwarding), mode(mode), pruneTimer(pruneTimer),
                  expiryTimer(expiryTimer), assert(assert), regState(regState), shRegTun(show) {}

            int getInterfaceId() const { return ie->getInterfaceId(); }
            virtual bool isEnabled() { return forwarding != Pruned; }
        };

    private:
        IPv4Address                 RP;                     /**< Randevous point */
        int                         flags;                  /**< Route flags */
        // PIMDM timers
        cMessage* graftRetryTimer;
        cMessage* sourceActiveTimer;
        cMessage* stateRefreshTimer;
        // PIMSM timers
        PIMkat                      *keepAliveTimer;
        PIMrst                      *registerStopTimer;
        PIMet                       *expiryTimer;
        PIMjt                       *joinTimer;
        PIMppt                      *prunePendingTimer;

        //Originated from destination.Ensures loop freeness.
        unsigned int sequencenumber;
        //Time of routing table entry creation
        simtime_t installtime;

    public:
        PIMMulticastRoute(IPv4Address origin, IPv4Address group);
        virtual ~PIMMulticastRoute() {}
        virtual std::string info() const;

        void setRP(IPv4Address RP)  {this->RP = RP;}                        /**< Set RP IP address */

        void setGraftRetryTimer (cMessage *grt)   {this->graftRetryTimer = grt;}
        void setSourceActiveTimer (cMessage *sat)   {this->sourceActiveTimer = sat;}
        void setStateRefreshTimer (cMessage *srt)   {this->stateRefreshTimer = srt;}

        void setKeepAliveTimer (PIMkat *kat)   {this->keepAliveTimer = kat;}
        void setRegisterStopTimer (PIMrst *rst)   {this->registerStopTimer = rst;}
        void setExpiryTimer  (PIMet *et)     {this->expiryTimer = et;}
        void setJoinTimer  (PIMjt *jt)     {this->joinTimer = jt;}
        void setPrunePendingTimer  (PIMppt *ppt)  {this->prunePendingTimer = ppt;}

        bool isFlagSet(Flag flag) const { return (flags & flag) != 0; }     /**< Returns if flag is set to entry or not*/
        void setFlags(int flags)   { this->flags |= flags; }                /**< Add flag to ineterface */
        void clearFlag(Flag flag)  { flags &= (~flag); }                   /**< Remove flag from ineterface */

        IPv4Address   getRP() const {return RP;}                            /**< Get RP IP address */

        cMessage*   getGraftRetryTimer() const {return graftRetryTimer;}
        cMessage*   getSourceActiveTimer() const {return sourceActiveTimer;}
        cMessage*   getStateRefreshTimer() const {return stateRefreshTimer;}

        PIMkat*     getKeepAliveTimer() const {return keepAliveTimer;}
        PIMrst*     getRegisterStopTimer() const {return registerStopTimer;}
        PIMet*      getExpiryTimer()  const {return expiryTimer;}
        PIMjt*      getJoinTimer()  const {return joinTimer;}
        PIMppt*     getPrunePendingTimer()  const {return prunePendingTimer;}

        // get incoming interface
        PIMInInterface *getPIMInInterface() const { return getInInterface() ? check_and_cast<PIMInInterface*>(getInInterface()) : NULL; }

        // get outgoing interface
        PIMOutInterface *getPIMOutInterface(unsigned int k) const { return getOutInterface(k) ? check_and_cast<PIMOutInterface*>(getOutInterface(k)) : NULL; }
        PIMOutInterface *findOutInterfaceByInterfaceId(int interfaceId);
        bool isOilistNull();                                                /**< Returns true if list of outgoing interfaces is empty, otherwise false*/

        simtime_t getInstallTime() const {return installtime;}
        void setInstallTime(simtime_t time) {installtime = time;}
        void setSequencenumber(int i){sequencenumber =i;}
        unsigned int getSequencenumber() const {return sequencenumber;}
};

#endif
