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
// Authors: Veronika Rybova, Vladimir Vesely (mailto:ivesely@fit.vutbr.cz)

#include "IPv4Datagram.h"
#include "PIMDM.h"

Define_Module(PIMDM);

using namespace std;

typedef IPv4MulticastRoute::OutInterface OutInterface;
typedef PIMMulticastRoute::PIMOutInterface PIMOutInterface;

void PIMDM::sendPrunePacket(IPv4Address nextHop, IPv4Address src, IPv4Address grp, int intId)
{
	EV_INFO << "Sending Prune(source = " << src << ", group = " << grp << ") message to neighbor '" << nextHop << "' on interface '" << intId << "'\n";

	PIMJoinPrune *packet = new PIMJoinPrune("PIMJoinPrune");
	packet->setUpstreamNeighborAddress(nextHop);
	packet->setHoldTime(pruneInterval);

	// set multicast groups
    packet->setMulticastGroupsArraySize(1);
	MulticastGroup &group = packet->getMulticastGroups(0);
	group.setGroupAddress(grp);
	group.setPrunedSourceAddressArraySize(1);
    EncodedAddress &address = group.getPrunedSourceAddress(0);
	address.IPaddress = src;

	sendToIP(packet, IPv4Address::UNSPECIFIED_ADDRESS, ALL_PIM_ROUTERS_MCAST, intId);
}

/*
 * PIM Graft messages use the same format as Join/Prune messages, except
 * that the Type field is set to 6.  The source address MUST be in the
 * Join section of the message.  The Hold Time field SHOULD be zero and
 * SHOULD be ignored when a Graft is received.
 */
void PIMDM::sendGraftPacket(IPv4Address nextHop, IPv4Address src, IPv4Address grp, int intId)
{
    EV_INFO << "Sending Graft(source = " << src << ", group = " << grp << ") message to neighbor '" << nextHop << "' on interface '" << intId << "'\n";

	PIMGraft *msg = new PIMGraft("PIMGraft");
	msg->setHoldTime(0);
	msg->setUpstreamNeighborAddress(nextHop);

    msg->setMulticastGroupsArraySize(1);
	MulticastGroup &group = msg->getMulticastGroups(0);
	group.setGroupAddress(grp);
	group.setJoinedSourceAddressArraySize(1);
    EncodedAddress &address = group.getJoinedSourceAddress(0);
    address.IPaddress = src;

	sendToIP(msg, IPv4Address::UNSPECIFIED_ADDRESS, nextHop, intId);
}

/*
 * PIM Graft Ack messages are identical in format to the received Graft
 * message, except that the Type field is set to 7.  The Upstream
 * Neighbor Address field SHOULD be set to the sender of the Graft
 * message and SHOULD be ignored upon receipt.
 */
void PIMDM::sendGraftAckPacket(PIMGraft *graftPacket)
{
    EV_INFO << "Sending GraftAck message.\n";

    IPv4ControlInfo *oldCtrl = check_and_cast<IPv4ControlInfo*>(graftPacket->removeControlInfo());
    IPv4Address destAddr = oldCtrl->getSrcAddr();
    IPv4Address srcAddr = oldCtrl->getDestAddr();
    int outInterfaceId = oldCtrl->getInterfaceId();
    delete oldCtrl;

    PIMGraftAck *msg = new PIMGraftAck();
    *((PIMGraft*)msg) = *graftPacket;
    msg->setName("PIMGraftAck");
    msg->setType(GraftAck);

    sendToIP(msg, srcAddr, destAddr, outInterfaceId);
}

void PIMDM::sendStateRefreshPacket(IPv4Address originator, IPv4Address src, IPv4Address grp, int intId, bool P)
{
    EV_INFO << "Sending StateRefresh(source = " << src << ", group = " << grp << ") message on interface '" << intId << "'\n";

	PIMStateRefresh *msg = new PIMStateRefresh("PIMStateRefresh");
	msg->setGroupAddress(grp);
	msg->setSourceAddress(src);
	msg->setOriginatorAddress(originator);
	msg->setInterval(stateRefreshInterval);
	msg->setP(P);

	sendToIP(msg, IPv4Address::UNSPECIFIED_ADDRESS, grp, intId);
}

void PIMDM::sendToIP(PIMPacket *packet, IPv4Address srcAddr, IPv4Address destAddr, int outInterfaceId)
{
    IPv4ControlInfo *ctrl = new IPv4ControlInfo();
    ctrl->setSrcAddr(srcAddr);
    ctrl->setDestAddr(destAddr);
    ctrl->setProtocol(IP_PROT_PIM);
    ctrl->setTimeToLive(1);
    ctrl->setInterfaceId(outInterfaceId);
    packet->setControlInfo(ctrl);
    send(packet, "ipOut");
}


/**
 * CREATE PRUNE TIMER
 *
 * The method is used to create PIMPrune timer. The timer is set when outgoing interface
 * goes to pruned state. After expiration (usually 3min) interface goes back to forwarding
 * state. It is set to (S,G,I).
 *
 * @param source IP address of multicast source.
 * @param group IP address of multicast group.
 * @param intId ID of outgoing interface.
 * @param holdTime Time of expiration (usually 3min).
 * @return Pointer to new Prune Timer.
 * @see PIMpt()
 */
cMessage* PIMDM::createPruneTimer(IPv4Address source, IPv4Address group, int intId, int holdTime)
{
    cMessage *timer = new cMessage("PimPruneTimer", PruneTimer);
    timer->setContextPointer(new TimerContext(source, group, intId));
	scheduleAt(simTime() + holdTime, timer);
	return timer;
}

/**
 * CREATE GRAFT RETRY TIMER
 *
 * The method is used to create PIMGraftRetry timer. The timer is set when router wants to join to
 * multicast tree again and send PIM Prune message to upstream. Router waits for Graft Retry Timer
 * (3 s) for PIM PruneAck message from upstream. If timer expires, router will send PIM Prune message
 * again. It is set to (S,G).
 *
 * @param source IP address of multicast source.
 * @param group IP address of multicast group.
 * @return Pointer to new Graft Retry Timer.
 * @see PIMgrt()
 */
cMessage* PIMDM::createGraftRetryTimer(IPv4Address source, IPv4Address group)
{
    cMessage *timer = new cMessage("PIMGraftRetryTimer", GraftRetryTimer);
	timer->setContextPointer(new TimerContext(source, group));
	scheduleAt(simTime() + graftRetryInterval, timer);
	return timer;
}

/**
 * CREATE SOURCE ACTIVE TIMER
 *
 * The method is used to create PIMSourceActive timer. The timer is set when source of multicast is
 * connected directly to the router.  If timer expires, router will remove the route from multicast
 * routing table. It is set to (S,G).
 *
 * @param source IP address of multicast source.
 * @param group IP address of multicast group.
 * @return Pointer to new Source Active Timer
 * @see PIMsat()
 */
cMessage *PIMDM::createSourceActiveTimer(IPv4Address source, IPv4Address group)
{
	cMessage *timer = new cMessage("PIMSourceActiveTimer", SourceActiveTimer);
	timer->setContextPointer(new TimerContext(source, group));
	scheduleAt(simTime() + sourceActiveInterval, timer);
	return timer;
}

/**
 * CREATE STATE REFRESH TIMER
 *
 * The method is used to create PIMStateRefresh timer. The timer is set when source of multicast is
 * connected directly to the router. If timer expires, router will send StateRefresh message, which
 * will propagate through all network and wil reset Prune Timer. It is set to (S,G).
 *
 * @param source IP address of multicast source.
 * @param group IP address of multicast group.
 * @return Pointer to new Source Active Timer
 * @see PIMsrt()
 */
cMessage *PIMDM::createStateRefreshTimer(IPv4Address source, IPv4Address group)
{
    cMessage *timer = new cMessage("PIMStateRefreshTimer", StateRefreshTimer);
	timer->setContextPointer(new TimerContext(source, group));
	scheduleAt(simTime() + stateRefreshInterval, timer);
	return timer;
}

void PIMDM::restartTimer(cMessage *timer, double interval)
{
    cancelEvent(timer);
    scheduleAt(simTime() + interval, timer);
}

/**
 * PROCESS GRAFT PACKET
 *
 * The method is used to process PIMGraft packet. Packet means that downstream router wants to join to
 * multicast tree, so the packet cannot come to RPF interface. Router finds correct outgoig interface
 * towards downstream router. Change its state to forward if it was not before and cancel Prune Timer.
 * If route was in pruned state, router will send also Graft message to join multicast tree.
 *
 * @param source IP address of multicast source.
 * @param group IP address of multicast group.
 * @param sender IP Address of Graft packet sender.
 * @param intId ID of Graft packet incoming interface.
 * @see sendPimGraft()
 * @see createGraftRetryTimer()
 * @see PIMGraft()
 * @see PIMgrt()
 * @see processJoinPruneGraftPacket()
 */
void PIMDM::processGraft(IPv4Address source, IPv4Address group, IPv4Address sender, int intId)
{
	EV_DEBUG << "Processing Graft, source=" << source << ", group=" << group << ", sender=" << sender << "incoming if=" << intId << endl;

	PIMMulticastRoute *route = getRouteFor(group, source);
	UpstreamInterface *upstream = check_and_cast<UpstreamInterface*>(route->getInInterface());

	// check if message come to non-RPF interface
	if (upstream->getInterfaceId() == intId)
	{
		EV << "ERROR: Graft message came to RPF interface." << endl;
		return;
	}

	// find outgoing interface to neighbor
    DownstreamInterface *downstream = dynamic_cast<DownstreamInterface*>(route->findOutInterfaceByInterfaceId(intId));
    if (downstream)
    {
        if (downstream->forwarding == PIMMulticastRoute::Pruned)
        {
            EV << "Interface " << downstream->getInterfaceId() << " transit to forwarding state (Graft)." << endl;
            downstream->forwarding = PIMMulticastRoute::Forward;

            //cancel Prune Timer
            if (downstream->pruneTimer)
            {
                cancelAndDeleteTimer(downstream->pruneTimer);
                downstream->pruneTimer = NULL;
            }
        }
    }

	// if all route was pruned, remove prune flag
	// if upstrem is not source, send Graft message
	if (route->isFlagSet(PIMMulticastRoute::P) && downstream && !upstream->graftRetryTimer)
	{
		if (!route->isFlagSet(PIMMulticastRoute::A))
		{
			EV << "Route is not pruned any more, send Graft to upstream" << endl;
			UpstreamInterface *inInterface = check_and_cast<UpstreamInterface*>(route->getInInterface());
			sendGraftPacket(inInterface->nextHop, source, group, inInterface->getInterfaceId());
			upstream->graftRetryTimer = createGraftRetryTimer(source, group);
		}
		else
			route->clearFlag(PIMMulticastRoute::P);
	}
}

/**
 * PROCESS PRUNE PACKET
 *
 * The method process PIM Prune packet. First the method has to find correct outgoing interface
 * where PIM Prune packet came to. The method also checks if there is still any forwarding outgoing
 * interface. Forwarding interfaces, where Prune packet come to, goes to prune state. If all outgoing
 * interfaces are pruned, the router will prune from multicast tree.
 *
 * @param route Pointer to multicast route which GraftAck belongs to.
 * @param intId
 * @see processJoinPruneGraftPacket()
 * @see createPruneTimer()
 * @see sendPimJoinPrune()
 * @see PIMpt()
 */
void PIMDM::processPrune(PIMMulticastRoute *route, int intId, int holdTime)
{
	EV_INFO << "Processing Prune " << endl;

	// we find correct outgoing interface
    DownstreamInterface *outInt = check_and_cast<DownstreamInterface*>(route->findOutInterfaceByInterfaceId(intId));
    if (!outInt)
        return;

    // if interface was already pruned, restart Prune Timer
    if (outInt->forwarding == PIMMulticastRoute::Pruned)
    {
        EV << "Outgoing interface is already pruned, restart Prune Timer." << endl;
        restartTimer(outInt->pruneTimer, holdTime);
    }
    // if interface is forwarding, transit its state to pruned and set Prune timer
    else
    {
        EV << "Outgoing interfaces is forwarding now -> change to Pruned." << endl;
        outInt->forwarding = PIMMulticastRoute::Pruned;
        outInt->pruneTimer = createPruneTimer(route->getOrigin(), route->getMulticastGroup(), intId, holdTime);

        // if there is no forwarding outgoing int, transit route to pruned state
        if (route->isOilistNull())
        {
            EV << "Route is not forwarding any more, send Prune to upstream." << endl;
            route->setFlags(PIMMulticastRoute::P);

            // if GRT is running now, do not send Prune msg
            UpstreamInterface *upstream = check_and_cast<UpstreamInterface*>(route->getInInterface());
            if (route->isFlagSet(PIMMulticastRoute::P) && upstream->graftRetryTimer)
            {
                cancelAndDeleteTimer(upstream->graftRetryTimer);
                upstream->graftRetryTimer = NULL;
            }
            else if (!route->isFlagSet(PIMMulticastRoute::A))
            {
                UpstreamInterface *upstream = check_and_cast<UpstreamInterface*>(route->getInInterface());
                sendPrunePacket(upstream->nextHop, route->getOrigin(), route->getMulticastGroup(), upstream->getInterfaceId());
            }
        }
    }
}

void PIMDM::processJoinPrunePacket(PIMJoinPrune *pkt)
{
    EV_INFO << "Received JoinPrune packet.\n";

    IPv4ControlInfo *ctrlInfo = check_and_cast<IPv4ControlInfo*>(pkt->getControlInfo());
    IPv4Address sender = ctrlInfo->getSrcAddr();
    InterfaceEntry *rpfInterface = rt->getInterfaceForDestAddr(sender);

    // does packet belong to this router?
    if (!rpfInterface || pkt->getUpstreamNeighborAddress() != rpfInterface->ipv4Data()->getIPAddress())
    {
        delete pkt;
        return;
    }

    int numRpfNeighbors = pimNbt->getNumNeighborsOnInterface(rpfInterface->getInterfaceId());

    // go through list of multicast groups
    for (unsigned int i = 0; i < pkt->getMulticastGroupsArraySize(); i++)
    {
        MulticastGroup group = pkt->getMulticastGroups(i);
        IPv4Address groupAddr = group.getGroupAddress();

        // go through list of joined sources
        for (unsigned int j = 0; j < group.getJoinedSourceAddressArraySize(); j++)
        {
            //FIXME join action
            // only if there is more than one PIM neighbor on one interface
            // interface change to forwarding state
            // cancel Prune Timer
            // send Graft to upstream
        }

        // go through list of pruned sources
        for(unsigned int k = 0; k < group.getPrunedSourceAddressArraySize(); k++)
        {
            EncodedAddress &source = group.getPrunedSourceAddress(k);
            PIMMulticastRoute *route = getRouteFor(groupAddr, source.IPaddress);

            // if there could be more than one PIM neighbor on interface
            if (numRpfNeighbors > 1)
            {
                ; //FIXME set PPT timer
            }
            // if there is only one PIM neighbor on interface
            else
                processPrune(route, rpfInterface->getInterfaceId(), pkt->getHoldTime());
        }
    }

    delete pkt;
}

void PIMDM::processGraftPacket(PIMGraft *pkt)
{
    EV_INFO << "Received Graft packet.\n";

    IPv4ControlInfo *ctrl = check_and_cast<IPv4ControlInfo*>(pkt->getControlInfo());
    IPv4Address sender = ctrl->getSrcAddr();
    InterfaceEntry * rpfInterface = rt->getInterfaceForDestAddr(sender);

    // does packet belong to this router?
    if (pkt->getUpstreamNeighborAddress() != rpfInterface->ipv4Data()->getIPAddress())
    {
        delete pkt;
        return;
    }

    for (unsigned int i = 0; i < pkt->getMulticastGroupsArraySize(); i++)
    {
        MulticastGroup &group = pkt->getMulticastGroups(i);
        IPv4Address groupAddr = group.getGroupAddress();

        for (unsigned int j = 0; j < group.getJoinedSourceAddressArraySize(); j++)
        {
            EncodedAddress &source = group.getJoinedSourceAddress(j);
            processGraft(source.IPaddress, groupAddr, sender, rpfInterface->getInterfaceId());
        }
    }

    // Send GraftAck for this Graft message
    sendGraftAckPacket(pkt);

    delete pkt;
}

void PIMDM::processGraftAckPacket(PIMGraftAck *pkt)
{
    EV_INFO << "Received GraftAck packet.\n";

    for (unsigned int i = 0; i < pkt->getMulticastGroupsArraySize(); i++)
    {
        MulticastGroup &group = pkt->getMulticastGroups(i);
        IPv4Address groupAddr = group.getGroupAddress();

        for (unsigned int j = 0; j < group.getJoinedSourceAddressArraySize(); j++)
        {
            EncodedAddress &source = group.getJoinedSourceAddress(j);
            PIMMulticastRoute *route = getRouteFor(groupAddr, source.IPaddress);

            UpstreamInterface *upstream = check_and_cast<UpstreamInterface*>(route->getInInterface());
            if (upstream->graftRetryTimer)
            {
                cancelAndDeleteTimer(upstream->graftRetryTimer);
                upstream->graftRetryTimer = NULL;
                route->clearFlag(PIMMulticastRoute::P);
            }
        }
    }

    delete pkt;
}


/**
 * PROCESS STATE REFRESH PACKET
 *
 * The method is used to process PIMStateRefresh packet. The method checks if there is route in mroute
 * and that packet has came to RPF interface. Then it goes through all outgoing interfaces. If the
 * interface is pruned, it resets Prune Timer. For each interface State Refresh message is copied and
 * correct prune indicator is set according to state of outgoing interface (pruned/forwarding).
 *
 * State Refresh message is used to stop flooding of network each 3 minutes.
 *
 * @param pkt Pointer to packet.
 * @see sendPimJoinPrune()
 * @see sendPimStateRefresh()
 */
void PIMDM::processStateRefreshPacket(PIMStateRefresh *pkt)
{
	EV << "pimDM::processStateRefreshPacket" << endl;

	// FIXME actions of upstream automat according to pruned/forwarding state and Prune Indicator from msg

	// first check if there is route for given group address and source
	PIMMulticastRoute *route = getRouteFor(pkt->getGroupAddress(), pkt->getSourceAddress());
	if (route == NULL)
	{
		delete pkt;
		return;
	}
	bool pruneIndicator;

	// chceck if State Refresh msg has came to RPF interface
	IPv4ControlInfo *ctrl = check_and_cast<IPv4ControlInfo*>(pkt->getControlInfo());
	UpstreamInterface *inInterface = check_and_cast<UpstreamInterface*>(route->getInInterface());
	if (ctrl->getInterfaceId() != inInterface->getInterfaceId())
	{
		delete pkt;
		return;
	}

	// this router is pruned, but outgoing int of upstream router leading to this router is forwarding
	if (route->isFlagSet(PIMMulticastRoute::P) && !pkt->getP())
	{
		// send Prune msg to upstream
		if (!inInterface->graftRetryTimer)
		{
		    UpstreamInterface *inInterface = check_and_cast<UpstreamInterface*>(route->getInInterface());
			sendPrunePacket(inInterface->nextHop, route->getOrigin(), route->getMulticastGroup(), inInterface->getInterfaceId());
		}
		else
		{
			cancelEvent(inInterface->graftRetryTimer);
			delete inInterface->graftRetryTimer;
			inInterface->graftRetryTimer = NULL;
			///////delete P
		}
	}

	// go through all outgoing interfaces, reser Prune Timer and send out State Refresh msg
	for (unsigned int i = 0; i < route->getNumOutInterfaces(); i++)
	{
	    DownstreamInterface *outInt = check_and_cast<DownstreamInterface*>(route->getPIMOutInterface(i));
		if (outInt->forwarding == PIMMulticastRoute::Pruned)
		{
			// P = true
			pruneIndicator = true;
			// reset PT
			cancelEvent(outInt->pruneTimer);
			scheduleAt(simTime() + pruneInterval, outInt->pruneTimer);
		}
		else if (outInt->forwarding == PIMMulticastRoute::Forward)
		{
			// P = false
			pruneIndicator = false;
		}
		sendStateRefreshPacket(pkt->getOriginatorAddress(), pkt->getSourceAddress(), pkt->getGroupAddress(), outInt->getInterfaceId(), pruneIndicator);
	}
	delete pkt;
}

void PIMDM::processAssertPacket(PIMAssert *pkt)
{
    EV_INFO << "Received Assert packet.\n";

    ASSERT(false); // not yet implemented

    delete pkt;
}

/*
 * The method is used to process PIM Prune timer. It is (S,G,I) timer. When Prune timer expires, it
 * means that outgoing interface transits back to forwarding state. If the router is pruned from
 * multicast tree, join again.
 */
void PIMDM::processPruneTimer(cMessage *timer)
{
	EV_INFO << "PruneTimer expired.\n";

	TimerContext *context = static_cast<TimerContext*>(timer->getContextPointer());
	IPv4Address source = context->source;
	IPv4Address group = context->group;
	int intId = context->interfaceId;

	// find correct (S,G) route which timer belongs to
	PIMMulticastRoute *route = getRouteFor(group, source);
	ASSERT(route);

	// state of interface is changed to forwarding
    DownstreamInterface *downstream = check_and_cast<DownstreamInterface*>(route->findOutInterfaceByInterfaceId(intId));
	if (downstream)
	{
	    ASSERT(timer == downstream->pruneTimer);

	    delete context;
		delete timer;
		downstream->pruneTimer = NULL;
		downstream->forwarding = PIMMulticastRoute::Forward;
		route->clearFlag(PIMMulticastRoute::P);

		// if the router is pruned from multicast tree, join again
		/*if (route->isFlagSet(P) && (route->getGrt() == NULL))
		{
			if (!route->isFlagSet(A))
			{
				EV << "Pruned cesta prejde do forwardu, posli Graft" << endl;
				sendPimGraft(route->getInIntNextHop(), source, group, route->getInIntId());
				PIMgrt* timer = createGraftRetryTimer(source, group);
				route->setGrt(timer);
			}
			else
				route->removeFlag(P);
		}*/
	}
}

void PIMDM::processGraftRetryTimer(cMessage *timer)
{
    EV_INFO << "GraftRetryTimer expired.\n";

    // send Graft message to upstream router
    TimerContext *context = static_cast<TimerContext*>(timer->getContextPointer());
	PIMMulticastRoute *route = getRouteFor(context->group, context->source);
	UpstreamInterface *upstream = check_and_cast<UpstreamInterface*>(route->getInInterface());
	sendGraftPacket(upstream->nextHop, context->source, context->group, upstream->getInterfaceId());
    scheduleAt(simTime() + graftRetryInterval, timer);
}

void PIMDM::processSourceActiveTimer(cMessage * timer)
{
    EV_INFO << "SourceActiveTimer expired.\n";

    // delete the route, because there are no more packets
	TimerContext *context = static_cast<TimerContext*>(timer->getContextPointer());
	PIMMulticastRoute *route = getRouteFor(context->group, context->source);
	rt->deleteMulticastRoute(route);
}

/*
 * State Refresh Timer is used only on router which is connected directly to the source of multicast.
 * When State Refresh Timer expires, State Refresh messages are sent on all downstream interfaces.
 */
void PIMDM::processStateRefreshTimer(cMessage *timer)
{
	EV_INFO << "StateRefreshTimer expired, sending StateRefresh packets on downstream interfaces.\n";

	TimerContext *context = static_cast<TimerContext*>(timer->getContextPointer());
	PIMMulticastRoute *route = getRouteFor(context->group, context->source);

	for (unsigned int i = 0; i < route->getNumOutInterfaces(); i++)
	{
	    DownstreamInterface *downstream = check_and_cast<DownstreamInterface*>(route->getPIMOutInterface(i));
	    bool isPruned = downstream->forwarding == PIMMulticastRoute::Pruned;
	    if (isPruned)
			restartTimer(downstream->pruneTimer, pruneInterval);

	    IPv4Address originator = downstream->getInterface()->ipv4Data()->getIPAddress();
		sendStateRefreshPacket(originator, context->source, context->group, downstream->getInterfaceId(), isPruned);
	}

    scheduleAt(simTime() + stateRefreshInterval, timer);
}

void PIMDM::processPIMTimer(cMessage *timer)
{
	EV << "pimDM::processPIMTimer: ";

	switch(timer->getKind())
	{
	    case HelloTimer:
	        processHelloTimer(timer);
           break;
		case AssertTimer:
			EV << "AssertTimer" << endl;
			break;
		case PruneTimer:
			EV << "PruneTimer" << endl;
			processPruneTimer(timer);
			break;
		case PrunePendingTimer:
			EV << "PrunePendingTimer" << endl;
			break;
		case GraftRetryTimer:
			EV << "GraftRetryTimer" << endl;
			processGraftRetryTimer(timer);
			break;
		case UpstreamOverrideTimer:
			EV << "UpstreamOverrideTimer" << endl;
			break;
		case PruneLimitTimer:
			EV << "PruneLimitTimer" << endl;
			break;
		case SourceActiveTimer:
			EV << "SourceActiveTimer" << endl;
			processSourceActiveTimer(timer);
			break;
		case StateRefreshTimer:
			EV << "StateRefreshTimer" << endl;
			processStateRefreshTimer(timer);
			break;
		default:
			EV << "BAD TYPE, DROPPED" << endl;
			delete timer;
			break;
	}
}

void PIMDM::processPIMPacket(PIMPacket *pkt)
{
	switch(pkt->getType())
	{
	    case Hello:
	        processHelloPacket(check_and_cast<PIMHello*>(pkt));
	        break;
		case JoinPrune:
			processJoinPrunePacket(check_and_cast<PIMJoinPrune*>(pkt));
			break;
		case Assert:
			processAssertPacket(check_and_cast<PIMAssert*>(pkt));
			break;
		case Graft:
			processGraftPacket(check_and_cast<PIMGraft*>(pkt));
			break;
		case GraftAck:
			processGraftAckPacket(check_and_cast<PIMGraftAck*>(pkt));
			break;
		case StateRefresh:
			processStateRefreshPacket(check_and_cast<PIMStateRefresh *> (pkt));
			break;
		default:
			EV_WARN << "Dropping packet " << pkt->getName() << ".\n";
			delete pkt;
			break;
	}
}


/**
 * HANDLE MESSAGE
 *
 * The method is used to handle new messages. Self messages are timer and they are sent to
 * method which processes PIM timers. Other messages should be PIM packets, so they are sent
 * to method which processes PIM packets.
 *
 * @param msg Pointer to new message.
 * @see PIMPacket()
 * @see PIMTimer()
 * @see processPIMTimer()
 * @see processPIMPkt()
 */
void PIMDM::handleMessage(cMessage *msg)
{
	// self message (timer)
   if (msg->isSelfMessage())
   {
	   EV << "PIMDM::handleMessage:Timer" << endl;
	   processPIMTimer(msg);
   }
   // PIM packet from PIM neighbor
   else if (dynamic_cast<PIMPacket *>(msg))
   {
	   EV << "PIMDM::handleMessage: PIM-DM packet" << endl;
	   PIMPacket *pkt = check_and_cast<PIMPacket *>(msg);
	   processPIMPacket(pkt);
   }
   // wrong message, mistake
   else
	   EV << "PIMDM::handleMessage: Wrong message" << endl;
}

void PIMDM::initialize(int stage)
{
    PIMBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        pruneInterval = par("pruneInterval");
        pruneLimitInterval = par("pruneLimitInterval");
        graftRetryInterval = par("graftRetryInterval");
        sourceActiveInterval = par("sourceActiveInterval");
        stateRefreshInterval = par("stateRefreshInterval");
    }
    else if (stage == INITSTAGE_ROUTING_PROTOCOLS)
	{
        // subscribe for notifications
        cModule *host = findContainingNode(this);
        if (host != NULL)
        {
            host->subscribe(NF_IPv4_NEW_MULTICAST, this);
            host->subscribe(NF_IPv4_MCAST_REGISTERED, this);
            host->subscribe(NF_IPv4_MCAST_UNREGISTERED, this);
            host->subscribe(NF_IPv4_DATA_ON_NONRPF, this);
            host->subscribe(NF_IPv4_DATA_ON_RPF, this);
            //host->subscribe(NF_IPv4_RPF_CHANGE, this);
            host->subscribe(NF_ROUTE_ADDED, this);
            host->subscribe(NF_INTERFACE_STATE_CHANGED, this);
        }
	}
}

void PIMDM::receiveSignal(cComponent *source, simsignal_t signalID, cObject *details)
{
	Enter_Method_Silent();
    printNotificationBanner(signalID, details);
	IPv4Datagram *datagram;
	PIMInterface *pimInterface;

    // new multicast data appears in router
    if (signalID == NF_IPv4_NEW_MULTICAST)
    {
        EV <<  "PimDM::receiveChangeNotification - NEW MULTICAST" << endl;
        datagram = check_and_cast<IPv4Datagram*>(details);
        IPv4Address srcAddr = datagram->getSrcAddress();
        IPv4Address destAddr = datagram->getDestAddress();
        unroutableMulticastPacketArrived(srcAddr, destAddr);
    }
    // configuration of interface changed, it means some change from IGMP, address were added.
    else if (signalID == NF_IPv4_MCAST_REGISTERED)
    {
        EV << "pimDM::receiveChangeNotification - IGMP change - address were added." << endl;
        IPv4MulticastGroupInfo *info = check_and_cast<IPv4MulticastGroupInfo*>(details);
        pimInterface = pimIft->getInterfaceById(info->ie->getInterfaceId());
        if (pimInterface && pimInterface->getMode() == PIMInterface::DenseMode)
            multicastReceiverAdded(pimInterface, info->groupAddress);
    }
    // configuration of interface changed, it means some change from IGMP, address were removed.
    else if (signalID == NF_IPv4_MCAST_UNREGISTERED)
    {
        EV << "pimDM::receiveChangeNotification - IGMP change - address were removed." << endl;
        IPv4MulticastGroupInfo *info = check_and_cast<IPv4MulticastGroupInfo*>(details);
        pimInterface = pimIft->getInterfaceById(info->ie->getInterfaceId());
        if (pimInterface && pimInterface->getMode() == PIMInterface::DenseMode)
            multicastReceiverRemoved(pimInterface, info->groupAddress);
    }
    // data come to non-RPF interface
    else if (signalID == NF_IPv4_DATA_ON_NONRPF)
    {
        EV << "pimDM::receiveChangeNotification - Data appears on non-RPF interface." << endl;
        datagram = check_and_cast<IPv4Datagram*>(details);
        pimInterface = getIncomingInterface(datagram);
        multicastPacketArrivedOnNonRpfInterface(datagram->getDestAddress(), datagram->getSrcAddress(), pimInterface? pimInterface->getInterfaceId():-1);
    }
    // data come to RPF interface
    else if (signalID == NF_IPv4_DATA_ON_RPF)
    {
        EV << "pimDM::receiveChangeNotification - Data appears on RPF interface." << endl;
        datagram = check_and_cast<IPv4Datagram*>(details);
        pimInterface = getIncomingInterface(datagram);
        if (pimInterface && pimInterface->getMode() == PIMInterface::DenseMode)
            multicastPacketArrivedOnRpfInterface(datagram->getDestAddress(), datagram->getSrcAddress(), pimInterface ? pimInterface->getInterfaceId():-1);
    }
    // RPF interface has changed
    else if (signalID == NF_ROUTE_ADDED)
    {
        EV << "pimDM::receiveChangeNotification - RPF interface has changed." << endl;
        IPv4Route *entry = check_and_cast<IPv4Route*>(details);
        IPv4Address routeSource = entry->getDestination();
        IPv4Address routeNetmask = entry->getNetmask();

        int numRoutes = rt->getNumMulticastRoutes();
        for (int i = 0; i < numRoutes; i++)
        {
            // find multicast routes whose source are on the destination of the new unicast route
            PIMMulticastRoute *route = dynamic_cast<PIMMulticastRoute*>(rt->getMulticastRoute(i));
            if (route && IPv4Address::maskedAddrAreEqual(route->getOrigin(), routeSource, routeNetmask))
            {
                IPv4Address source = route->getOrigin();
                InterfaceEntry *newRpfInterface = rt->getInterfaceForDestAddr(source);
                UpstreamInterface *oldRpfInterface = check_and_cast<UpstreamInterface*>(route->getInInterface());

                // is there any change?
                if (newRpfInterface != oldRpfInterface->getInterface())
                    rpfInterfaceHasChanged(route, newRpfInterface);
            }
        }
    }
}

/**
 * RPF INTERFACE CHANGE
 *
 * The method process notification about interface change. Multicast routing table will be
 * changed if RPF interface has changed. New RPF interface is set to route and is removed
 * from outgoing interfaces. On the other hand, old RPF interface is added to outgoing
 * interfaces. If route was not pruned, the router has to join to the multicast tree again
 * (by different path).
 *
 * @param newRoute Pointer to new entry in the multicast routing table.
 * @see sendPimGraft()
 * @see createGraftRetryTimer()
 */
void PIMDM::rpfInterfaceHasChanged(PIMMulticastRoute *route, InterfaceEntry *newRpf)
{
	IPv4Address source = route->getOrigin();
	IPv4Address group = route->getMulticastGroup();
	int rpfId = newRpf->getInterfaceId();
	UpstreamInterface *inInterface = check_and_cast<UpstreamInterface*>(route->getInInterface());

	EV << "New RPF int for group " << group << " source " << source << " is " << rpfId << endl;

	// set new RPF
	InterfaceEntry *oldRpfInt = route->getInInterface() ? route->getInInterface()->getInterface() : NULL;
	UpstreamInterface *newInInterface = new UpstreamInterface(this, newRpf, pimNbt->getNeighborsOnInterface(rpfId)[0]->getAddress());
	route->setInInterface(newInInterface);

	// route was not pruned, join to the multicast tree again
	if (!route->isFlagSet(PIMMulticastRoute::P))
	{
		sendGraftPacket(newInInterface->nextHop, source, group, rpfId);
		inInterface->graftRetryTimer = createGraftRetryTimer(source, group);
	}

	// find rpf int in outgoing imterfaces and delete it
	for(unsigned int i = 0; i < route->getNumOutInterfaces(); i++)
	{
	    DownstreamInterface *outInt = check_and_cast<DownstreamInterface*>(route->getPIMOutInterface(i));
		if (outInt->getInterfaceId() == rpfId)
		{
			if (outInt->pruneTimer != NULL)
			{
				cancelEvent(outInt->pruneTimer);
				delete outInt->pruneTimer;
				outInt->pruneTimer = NULL;
			}
			route->removeOutInterface(i);
			break;
		}
	}

	// old RPF should be now outgoing interface if it is not down
	if (oldRpfInt && oldRpfInt->isUp())
	{
	    DownstreamInterface *newOutInt = new DownstreamInterface(this, oldRpfInt);
		newOutInt->pruneTimer = NULL;
		newOutInt->forwarding = PIMMulticastRoute::Forward;
		newOutInt->mode = PIMInterface::DenseMode;
		route->addOutInterface(newOutInt);
	}
}

void PIMDM::multicastPacketArrivedOnRpfInterface(IPv4Address group, IPv4Address source, int interfaceId)
{
    PIMMulticastRoute *route = getRouteFor(group, source);
    UpstreamInterface *upstream = check_and_cast<UpstreamInterface*>(route->getInInterface());
	restartTimer(upstream->sourceActiveTimer, sourceActiveInterval);

    if (route->getNumOutInterfaces() == 0 || route->isFlagSet(PIMMulticastRoute::P))
    {
        EV << "Route does not have any outgoing interface or it is pruned." << endl;

        EV << "pimDM::dataOnPruned" << endl;
        if (route->isFlagSet(PIMMulticastRoute::A))
            return;

        // if GRT is running now, do not send Prune msg
        if (route->isFlagSet(PIMMulticastRoute::P) && upstream->graftRetryTimer)
        {
            cancelAndDeleteTimer(upstream->graftRetryTimer);
            upstream->graftRetryTimer = NULL;
        }
        // otherwise send Prune msg to upstream router
        else
        {
            sendPrunePacket(upstream->nextHop, source, group, upstream->getInterfaceId());
        }
    }
}

/**
 * DATA ON NON-RPF INTERFACE
 *
 * The method has to solve the problem when multicast data appears on non-RPF interface. It can
 * happen when there is loop in the network. In this case, router has to prune from the neighbor,
 * so it sends Prune message.
 *
 * @param group Multicast group IP address.
 * @param source Source IP address.
 * @param intID Identificator of incoming interface.
 * @see sendPimJoinPrune()
 * @see createPruneTimer()
 */
void PIMDM::multicastPacketArrivedOnNonRpfInterface(IPv4Address group, IPv4Address source, int intId)
{
	EV << "pimDM::dataOnNonRpf, intID: " << intId << endl;

	// load route from mroute
	PIMMulticastRoute *route = getRouteFor(group, source);
	if (route == NULL)
		return;

	// in case of p2p link, send prune
	// FIXME There should be better indicator of P2P link
	if (pimNbt->getNumNeighborsOnInterface(intId) == 1)
	{
		// send Prune msg to the neighbor who sent these multicast data
		IPv4Address nextHop = (pimNbt->getNeighborsOnInterface(intId))[0]->getAddress();
		sendPrunePacket(nextHop, source, group, intId);

		// the incoming interface has to change its state to Pruned
		DownstreamInterface *outInt = check_and_cast<DownstreamInterface*>(route->findOutInterfaceByInterfaceId(intId));
		if (outInt && outInt->forwarding == PIMMulticastRoute::Forward)
		{
			outInt->forwarding = PIMMulticastRoute::Pruned;
			outInt->pruneTimer = createPruneTimer(route->getOrigin(), route->getMulticastGroup(), intId, pruneInterval);

			// if there is no outgoing interface, Prune msg has to be sent on upstream
			if (route->isOilistNull())
			{
				EV << "Route is not forwarding any more, send Prune to upstream" << endl;
				route->setFlags(PIMMulticastRoute::P);
				if (!route->isFlagSet(PIMMulticastRoute::A))
				{
				    UpstreamInterface *inInterface = check_and_cast<UpstreamInterface*>(route->getInInterface());
					sendPrunePacket(inInterface->nextHop, route->getOrigin(), route->getMulticastGroup(), inInterface->getInterfaceId());
				}
			}
		}
	}

	//FIXME in case of LAN
}

/**
 * OLD MULTICAST ADDRESS
 *
 * The method process notification about multicast groups removed from interface. For each
 * old address it tries to find route. If there is route, it finds interface in list of outgoing
 * interfaces. If the interface is in the list it will be removed. If the router was not pruned
 * and there is no outgoing interface, the router will prune from the multicast tree.
 *
 * @param members Structure containing old multicast IP addresses.
 * @see sendPimJoinPrune()
 */
void PIMDM::multicastReceiverRemoved(PIMInterface *pimInt, IPv4Address oldAddr)
{
	EV << "pimDM::oldMulticastAddr" << endl;
	bool connected = false;

	// go through all old multicast addresses assigned to interface
    EV << "Removed multicast address: " << oldAddr << endl;
    vector<PIMMulticastRoute*> routes = getRouteFor(oldAddr);

    // go through all multicast routes
    for (unsigned int j = 0; j < routes.size(); j++)
    {
        PIMMulticastRoute *route = routes[j];
        unsigned int k;

        // is interface in list of outgoing interfaces?
        for (k = 0; k < route->getNumOutInterfaces();)
        {
            PIMOutInterface *outInt = route->getPIMOutInterface(k);
            if (outInt->getInterfaceId() == pimInt->getInterfaceId())
            {
                EV << "Interface is present, removing it from the list of outgoing interfaces." << endl;
                route->removeOutInterface(k);
            }
            else if(outInt->forwarding == PIMMulticastRoute::Forward)
            {
                if ((pimNbt->getNeighborsOnInterface(outInt->getInterfaceId())).size() == 0)
                    connected = true;
                k++;
            }
            else
                k++;
        }

        // if there is no directly connected member of group
        if (!connected)
            route->clearFlag(PIMMulticastRoute::C);

        // there is no receiver of multicast, prune the router from the multicast tree
        if (route->isOilistNull())
        {
            EV << "Route is not forwarding any more, send Prune to upstream" << endl;
            // if GRT is running now, do not send Prune msg
            UpstreamInterface *upstream = check_and_cast<UpstreamInterface*>(route->getInInterface());
            if (route->isFlagSet(PIMMulticastRoute::P) && upstream->graftRetryTimer)
            {
                cancelAndDeleteTimer(upstream->graftRetryTimer);
                upstream->graftRetryTimer = NULL;
                sendPrunePacket(upstream->nextHop, route->getOrigin(), route->getMulticastGroup(), upstream->getInterfaceId());
            }

            // if the source is not directly connected, sent Prune msg
            if (!route->isFlagSet(PIMMulticastRoute::A) && !route->isFlagSet(PIMMulticastRoute::P))
            {
                sendPrunePacket(upstream->nextHop, route->getOrigin(), route->getMulticastGroup(), upstream->getInterfaceId());
            }

            route->setFlags(PIMMulticastRoute::P);
        }
    }
}

/**
 * NEW MULTICAST ADDRESS
 *
 * The method process notification about new multicast groups aasigned to interface. For each
 * new address it tries to find route. If there is route, it finds interface in list of outgoing
 * interfaces. If the interface is not in the list it will be added. if the router was pruned
 * from multicast tree, join again.
 *
 * @param members Structure containing new multicast IP addresses.
 * @see sendPimGraft()
 * @see createGraftRetryTimer()
 * @see addRemoveAddr
 */
void PIMDM::multicastReceiverAdded(PIMInterface *pimInt, IPv4Address newAddr)
{
	EV << "pimDM::newMulticastAddr" << endl;
	bool forward = false;

    EV << "New multicast address: " << newAddr << endl;
    vector<PIMMulticastRoute*> routes = getRouteFor(newAddr);

    // go through all multicast routes
    for (unsigned int j = 0; j < routes.size(); j++)
    {
        PIMMulticastRoute *route = routes[j];
        //PIMMulticastRoute::AnsaOutInterfaceVector outInt = route->getOutInt();
        unsigned int k;

        // check on RPF interface
        UpstreamInterface *rpfInterface = check_and_cast<UpstreamInterface*>(route->getInInterface());
        if (rpfInterface->getInterface() == pimInt->getInterfacePtr())
            continue;

        // is interface in list of outgoing interfaces?
        for (k = 0; k < route->getNumOutInterfaces(); k++)
        {
            PIMOutInterface *outInt = route->getPIMOutInterface(k);
            if (outInt->getInterfaceId() == pimInt->getInterfaceId())
            {
                EV << "Interface is already on list of outgoing interfaces" << endl;
                if (outInt->forwarding == PIMMulticastRoute::Pruned)
                    outInt->forwarding = PIMMulticastRoute::Forward;
                forward = true;
                break;
            }
        }

        // interface is not in list of outgoing interfaces
        if (k == route->getNumOutInterfaces())
        {
            EV << "Interface is not on list of outgoing interfaces yet, it will be added" << endl;
            DownstreamInterface *newInt = new DownstreamInterface(this, pimInt->getInterfacePtr());
            newInt->mode = PIMInterface::DenseMode;
            newInt->forwarding = PIMMulticastRoute::Forward;
            newInt->pruneTimer = NULL;
            route->addOutInterface(newInt);
            forward = true;
        }
        route->setFlags(PIMMulticastRoute::C);

        // route was pruned, has to be added to multicast tree
        if (route->isFlagSet(PIMMulticastRoute::P) && forward)
        {
            EV << "Route is not pruned any more, send Graft to upstream" << endl;

            // if source is not directly connected, send Graft to upstream
            if (!route->isFlagSet(PIMMulticastRoute::A))
            {
                UpstreamInterface *inInterface = check_and_cast<UpstreamInterface*>(route->getInInterface());
                sendGraftPacket(inInterface->nextHop, route->getOrigin(), route->getMulticastGroup(), inInterface->getInterfaceId());
                inInterface->graftRetryTimer = createGraftRetryTimer(route->getOrigin(), route->getMulticastGroup());
            }
            else
                route->clearFlag(PIMMulticastRoute::P);
        }
    }
}

/**
 * NEW MULTICAST
 *
 * The method process notification about new multicast data stream. It goes through all PIM
 * interfaces and tests them if they can be added to the list of outgoing interfaces. If there
 * is no interface on the list at the end, the router will prune from the multicast tree.
 *
 * @param newRoute Pointer to new entry in the multicast routing table.
 * @see sendPimGraft()
 * @see createGraftRetryTimer()
 * @see addRemoveAddr
 */
void PIMDM::unroutableMulticastPacketArrived(IPv4Address srcAddr, IPv4Address destAddr)
{
    ASSERT(!srcAddr.isUnspecified());
    ASSERT(destAddr.isMulticast());

    EV << "pimDM::newMulticast" << endl;

	IPv4Route *routeToSrc = rt->findBestMatchingRoute(srcAddr);
	if (!routeToSrc || !routeToSrc->getInterface())
	{
        EV << "ERROR: PIMDM::newMulticast(): cannot find RPF interface, routing information is missing.";
        return;
	}

    PIMInterface *rpfInterface = pimIft->getInterfaceById(routeToSrc->getInterface()->getInterfaceId());
    if (!rpfInterface || rpfInterface->getMode() != PIMInterface::DenseMode)
        return;

    EV << "PIMDM::newMulticast - group: " << destAddr << ", source: " << srcAddr << endl;

    // gateway is unspecified for directly connected destinations
    IPv4Address rpfNeighbor = routeToSrc->getGateway().isUnspecified() ? srcAddr : routeToSrc->getGateway();

    // create new multicast route
    PIMMulticastRoute *newRoute = new PIMMulticastRoute(srcAddr, destAddr);
    newRoute->setSourceType(IMulticastRoute::PIM_DM);
    newRoute->setSource(this);
    UpstreamInterface *upstream = new UpstreamInterface(this, rpfInterface->getInterfacePtr(), rpfNeighbor);
    newRoute->setInInterface(upstream);
    if (routeToSrc->getSourceType() == IPv4Route::IFACENETMASK)
        newRoute->setFlags(PIMMulticastRoute::A);


	// only outgoing interfaces are missing
	bool pruned = true;

	// insert all PIM interfaces except rpf int
	for (int i = 0; i < pimIft->getNumInterfaces(); i++)
	{
		PIMInterface *pimIntTemp = pimIft->getInterface(i);
		int intId = pimIntTemp->getInterfaceId();

		//check if PIM interface is not RPF interface
		if (pimIntTemp == rpfInterface)
			continue;

		// create new outgoing interface
		DownstreamInterface *newOutInt = new DownstreamInterface(this, pimIntTemp->getInterfacePtr());
		newOutInt->pruneTimer = NULL;

		switch (pimIntTemp->getMode())
		{
			case PIMInterface::DenseMode:
				newOutInt->mode = PIMInterface::DenseMode;
				break;
			case PIMInterface::SparseMode:
				newOutInt->mode = PIMInterface::SparseMode;
				break;
		}

		// if there are neighbors on interface, we will forward
		if((pimNbt->getNeighborsOnInterface(intId)).size() > 0)
		{
			newOutInt->forwarding = PIMMulticastRoute::Forward;
			pruned = false;
			newRoute->addOutInterface(newOutInt);
		}
		// if there is member of group, we will forward
		else if (pimIntTemp->getInterfacePtr()->ipv4Data()->hasMulticastListener(newRoute->getMulticastGroup()))
		{
			newOutInt->forwarding = PIMMulticastRoute::Forward;
			pruned = false;
			newRoute->setFlags(PIMMulticastRoute::C);
			newRoute->addOutInterface(newOutInt);
		}
		// in any other case interface is not involved
	}

	// directly connected to source, set State Refresh Timer
	if (newRoute->isFlagSet(PIMMulticastRoute::A) && rpfInterface->getSR())
	{
	    upstream->stateRefreshTimer = createStateRefreshTimer(newRoute->getOrigin(), newRoute->getMulticastGroup());
	}

	// set Source Active Timer (liveness of route)
	upstream->sourceActiveTimer = createSourceActiveTimer(newRoute->getOrigin(), newRoute->getMulticastGroup());

	// if there is no outgoing interface, prune from multicast tree
	if (pruned)
	{
		EV << "pimDM::newMulticast: There is no outgoing interface for multicast, send Prune msg to upstream" << endl;
		newRoute->setFlags(PIMMulticastRoute::P);

		// Prune message is sent from the forwarding hook (NF_IPv4_DATA_ON_RPF), see dataOnRpf()
	}

	// add new route record to multicast routing table
	rt->addMulticastRoute(newRoute);

	EV_DETAIL << "New route was added to the multicast routing table.\n";
}

//----------------------------------------------------------------------------
//           Helpers
//----------------------------------------------------------------------------

PIMInterface *PIMDM::getIncomingInterface(IPv4Datagram *datagram)
{
    cGate *g = datagram->getArrivalGate();
    if (g)
    {
        InterfaceEntry *ie = g ? ift->getInterfaceByNetworkLayerGateIndex(g->getIndex()) : NULL;
        if (ie)
            return pimIft->getInterfaceById(ie->getInterfaceId());
    }
    return NULL;
}

void PIMDM::cancelAndDeleteTimer(cMessage *timer)
{
    if (timer)
    {
        cancelEvent(timer);
        delete static_cast<TimerContext*>(timer->getContextPointer());
        delete timer;
    }
}

PIMMulticastRoute *PIMDM::getRouteFor(IPv4Address group, IPv4Address source)
{
    int numRoutes = rt->getNumMulticastRoutes();
    for (int i = 0; i < numRoutes; i++)
    {
        PIMMulticastRoute *route = dynamic_cast<PIMMulticastRoute*>(rt->getMulticastRoute(i));
        if (route && route->getMulticastGroup() == group && route->getOrigin() == source)
            return route;
    }
    return NULL;
}

vector<PIMMulticastRoute*> PIMDM::getRouteFor(IPv4Address group)
{
    vector<PIMMulticastRoute*> routes;
    int numRoutes = rt->getNumMulticastRoutes();
    for (int i = 0; i < numRoutes; i++)
    {
        PIMMulticastRoute *route = dynamic_cast<PIMMulticastRoute*>(rt->getMulticastRoute(i));
        if (route && route->getMulticastGroup() == group)
            routes.push_back(route);
    }
    return routes;
}

PIMDM::UpstreamInterface::~UpstreamInterface()
{
    owner->cancelAndDeleteTimer(stateRefreshTimer);
    owner->cancelAndDeleteTimer(graftRetryTimer);
    owner->cancelAndDeleteTimer(sourceActiveTimer);
    owner->cancelAndDeleteTimer(overrideTimer);
}

PIMDM::DownstreamInterface::~DownstreamInterface()
{
    owner->cancelAndDeleteTimer(pruneTimer);
}
