//
// Copyright (C) 2008 Juan-Carlos Maureira
// Copyright (C) INRIA
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

#include "DHCPClient.h"

#include "InterfaceTableAccess.h"
#include "IPv4InterfaceData.h"
#include "ModuleAccess.h"
#include "NodeStatus.h"
#include "NotifierConsts.h"
#include "NodeOperations.h"
#include "IPv4RoutingTableAccess.h"

Define_Module(DHCPClient);

DHCPClient::DHCPClient()
{
    timer_t1 = NULL;
    timer_t2 = NULL;
    timer_to = NULL;
    host = NULL;
    ie = NULL;
    irt = NULL;
    lease = NULL;
}

DHCPClient::~DHCPClient()
{
    cancelTimer_T1();
    cancelTimer_T2();
    cancelTimer_TO();
}

void DHCPClient::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        timer_t1 = NULL;
        timer_t2 = NULL;
        timer_to = NULL;

        numSent = 0;
        numReceived = 0;
        retry_count = 0;
        xid = 0;

        retry_max = 10; // Resent attempts
        response_timeout = 1; // response timeout in seconds;

        WATCH(numSent);
        WATCH(numReceived);
        WATCH(retry_count);
        WATCH(client_state);
        WATCH(xid);

        // DHCP UDP ports
        bootpc_port = 68; // client
        bootps_port = 67; // server

        host = getContainingNode(this);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        bool isOperational;
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus *>(host->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
        if (!isOperational)
            throw cRuntimeError("This module doesn't support starting in node DOWN state");

        // get the hostname
        host_name = host->getFullName();

        // for a wireless interface subscribe the association event to start the DHCP protocol
        host->subscribe(NF_L2_ASSOCIATED, this);

        // Get the interface to configure
        IInterfaceTable* ift = check_and_cast<IInterfaceTable*>(getModuleByPath(par("interfaceTablePath")));
        ie = ift->getInterfaceByName(par("interface"));

        if (ie == NULL)
        {
            error("DHCP Interface does not exist. aborting.");
            return;
        }

        // get the routing table to update and subscribe it to the blackboard
        irt = check_and_cast<IIPv4RoutingTable *>(getModuleByPath(par("routingTableModule")));

        // grab the interface mac address
        client_mac_address = ie->getMacAddress();

        // bind the client to the udp port
        socket.setOutputGate(gate("udpOut"));
        socket.bind(bootpc_port);
        socket.setBroadcast(true);
        EV << "DHCP Client bound to port " << bootpc_port << " at " << ie->getName() <<  endl;

        // set client to idle state
        client_state = IDLE;

        // FIXME following line is a HACK. It allows to work with all type of interfaces (not just wireless)
        // a correct fix would need some kind of notification when the wireless interface is associated
        // or when the eth interface gets connected and would set the INIT state only then. At the moment
        // there is no such notification in INET.
        changeFSMState(INIT);
    }
}

void DHCPClient::finish()
{
    cancelTimer_T1();
    cancelTimer_T2();
    cancelTimer_TO();
}

namespace {

inline bool routeMatches(const IPv4Route *entry,
    const IPv4Address& target, const IPv4Address& nmask,
    const IPv4Address& gw, int metric, const char *dev)
{
    if (!target.isUnspecified() && !target.equals(entry->getDestination()))
        return false;
    if (!nmask.isUnspecified() && !nmask.equals(entry->getNetmask()))
        return false;
    if (!gw.isUnspecified() && !gw.equals(entry->getGateway()))
        return false;
    if (metric && metric!=entry->getMetric())
        return false;
    if (dev && strcmp(dev, entry->getInterfaceName()))
        return false;

    return true;
}

}

void DHCPClient::changeFSMState(CLIENT_STATE new_state)
{
    client_state = new_state;
    if (new_state == INIT)
    {

        cancelTimer_T1();
        cancelTimer_T2();
        cancelTimer_TO();

        changeFSMState(SELECTING);
    }
    else if (new_state == SELECTING)
    {
        // the selected lease is in lease
        sendDiscover();
        scheduleTimer_TO(WAIT_OFFER);
    }
    else if (new_state == REQUESTING)
    {
        // the selected lease is in lease
        sendRequest();
        scheduleTimer_TO(WAIT_ACK);
    }
    else if (new_state == BOUND)
    {
        cancelTimer_TO();
        scheduleTimer_T1();
        scheduleTimer_T2();

        // Assign the IP to the interface
        // TODO: client must remove the configured IP address when the lease expires
        ie->ipv4Data()->setIPAddress(lease->ip);
        ie->ipv4Data()->setNetmask(lease->netmask);

        std::string banner = "Got IP " + lease->ip.str();
        host->bubble(banner.c_str());

        EV << "Configuring interface : " << ie->getName() << " ip:" << lease->ip << "/"
           << lease->netmask << " leased time: " << lease->lease_time << " (secs)" << endl;
        std::cout << "Host " << host_name << " got ip: " << lease->ip << "/" << lease->netmask << endl;

        // XXX this code probably want to add a default route if there is no default route.
        //     However routeMatches() does not compare unspecified addresses, so in the loop
        //     we will find the first route via the gateway.
        IPv4Route *iroute = NULL;
        for (int i=0;i<irt->getNumRoutes();i++)
        {
            IPv4Route * e = irt->getRoute(i);
            if (routeMatches(e, IPv4Address(), IPv4Address(), lease->gateway, 0, (char*) (ie->getName())))
            {
                iroute =  e;
                break;
            }
        }
        if (iroute == NULL)
        {
            // create gateway route
            IPv4Route *e = new IPv4Route();
            e->setDestination(IPv4Address());
            e->setNetmask(IPv4Address());
            e->setGateway(lease->gateway);
            e->setInterface(ie);
            e->setSourceType(IRoute::MANUAL);
            irt->addRoute(e);
        }
    }
    else if (new_state == RENEWING)
    {
        // asking for lease renewal
        sendRequest();
        scheduleTimer_TO(WAIT_ACK);
    }
    else if (new_state == REBINDING)
    {
        // asking for lease rebinding
        cancelTimer_T1();
        cancelTimer_T2();
        cancelTimer_TO();

        sendRequest();
        scheduleTimer_TO(WAIT_ACK);
    }
}

void DHCPClient::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        handleTimer(msg);
    }
    else if (msg->arrivedOn("udpIn"))
    {
        DHCPMessage *dhcpPacket = dynamic_cast<DHCPMessage*>(msg);
        // check if the message is DHCPMessage
        if (dhcpPacket)
        {
            handleDHCPMessage(dhcpPacket);
        }
        else
        {
            EV << "unknown packet, discarding it" << endl;
        }
        // delete the msg
        delete msg;
    }
}

void DHCPClient::handleTimer(cMessage* msg)
{
    int category = msg->getKind();

    if (category == WAIT_OFFER)
    {
        if (retry_count < retry_max)
        {
            changeFSMState(SELECTING);
            retry_count++;
        }
        else
        {
            EV << "No DHCP offer. restarting. " << endl;
            retry_count = 0;
            changeFSMState(INIT);
        }
    }

    if (category == WAIT_ACK)
    {
        if (retry_count < retry_max)
        {
            // trigger the current state one more time
            retry_count++;
        }
        else
        {
            EV << "No DHCP ACK. restarting. " << endl;
            retry_count = 0;
        }
        if (client_state == REQUESTING)
        {
            changeFSMState(REQUESTING);
        }
        if (client_state == RENEWING)
        {
            changeFSMState(RENEWING);
        }
        if (client_state == REBINDING)
        {
            changeFSMState(REBINDING);
        }
    }

    if (category == T1)
    {
        EV << "T1 reached. starting RENEWING state " << endl;
        changeFSMState(RENEWING);
    }

    if (category == T2 && client_state == RENEWING)
    {
        EV << "T2 reached. starting REBINDING state " << endl;
        changeFSMState(REBINDING);
    }

}

void DHCPClient::handleDHCPMessage(DHCPMessage* msg)
{
    EV << "arrived DHCP message:" << msg << endl;

    // check the packet type (reply) and transaction id
    if (msg->getOp() == BOOTREPLY && msg->getXid() == xid)
    {
        // bootreply  is 4 us.
        if (msg->getOptions().get(DHCP_MSG_TYPE) == DHCPOFFER && client_state == SELECTING)
        {

            // the offering to our discover arrived
            if (!msg->getYiaddr().isUnspecified())
            {
                IPv4Address ip = msg->getYiaddr();
                EV << "DHCPOFFER arrived" << endl;

                Byte server_id_b = msg->getOptions().get(SERVER_ID);
                IPv4Address server_id;
                if (server_id_b.stringValue() != "")
                {
                    server_id = IPv4Address(server_id_b.stringValue().c_str());
                }

                // minimal information to configure the interface
                if (!ip.isUnspecified())
                {
                    // create the lease to request
                    lease = new DHCPLease();
                    lease->ip = ip;
                    lease->mac = client_mac_address;
                    lease->server_id = server_id;
                }
                // start requesting
                changeFSMState(REQUESTING);

            }
            else
            {
                EV << "DHCPOFFER arrived, but no ip address has been offered. discarding it and remains in SELECTING." << endl;
            }
        }

        // check if the msg is DHCP ACK and we are waiting the ACK
        if (msg->getOptions().get(DHCP_MSG_TYPE) == DHCPACK)
        {

            if (!msg->getYiaddr().isUnspecified())
            {
                IPv4Address ip = msg->getYiaddr();
                EV << "DHCPACK arrived" << endl << "IP: " << ip << endl;

                // extract the server_id
                Byte server_id_b = msg->getOptions().get(SERVER_ID);
                IPv4Address server_id;
                if (server_id_b.stringValue() != "")
                {
                    server_id = IPv4Address(server_id_b.stringValue().c_str());
                }

                Byte netmask_b(msg->getOptions().get(SUBNET_MASK));
                Byte gateway_b(msg->getOptions().get(ROUTER));
                Byte dns_b(msg->getOptions().get(ROUTER));
                Byte ntp_b(msg->getOptions().get(NTP_SRV));

                IPv4Address netmask;
                IPv4Address gateway;
                IPv4Address dns;
                IPv4Address ntp;

                if (netmask_b.intValue() > 0)
                {
                    lease->netmask = IPv4Address(netmask_b.stringValue().c_str());
                }

                if (gateway_b.intValue() > 0)
                {
                    lease->gateway = IPv4Address(gateway_b.stringValue().c_str());
                }
                if (dns_b.intValue() > 0)
                {
                    lease->dns = IPv4Address(dns_b.stringValue().c_str());
                }
                if (ntp_b.intValue() > 0)
                {
                    lease->ntp = IPv4Address(ntp_b.stringValue().c_str());
                }

                lease->lease_time = msg->getOptions().get(LEASE_TIME);
                lease->renewal_time = msg->getOptions().get(RENEWAL_TIME);
                lease->rebind_time = msg->getOptions().get(REBIND_TIME);

                // starting BOUND
                changeFSMState(BOUND);
            }
            else
            {
                EV << "DHCPACK arrived, but no ip confirmed." << endl;
            }
        }
    }
    else
    {
        EV << "dhcp message is not for us. discarding it" << endl;
    }
}

void DHCPClient::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    Enter_Method_Silent();
    printNotificationBanner(signalID, obj);

    // host associated. Link is up. Change the state to init.
    if (signalID == NF_L2_ASSOCIATED)
    {
        InterfaceEntry * ie = NULL;
        if (obj)
        {
            ie = dynamic_cast<InterfaceEntry*>(obj);
        }
        if (!ie || (ie && (ie == ie)))
        {
            EV << "Interface Associated, starting DHCP" << endl;
            changeFSMState(INIT);
        }
    }
}

void DHCPClient::sendRequest()
{

    // setting the xid
    xid = intuniform(0, RAND_MAX); //random();

    DHCPMessage* request = new DHCPMessage("DHCPREQUEST");
    request->setOp(BOOTREQUEST);
    request->setByteLength(280); // DHCP request packet size;
    request->setHtype(1); // Ethernet
    request->setHlen(6); // Hardware Address lenght (6 octets)
    request->setHops(0);
    request->setXid(xid); // transacction id;
    request->setSecs(0); // 0 seconds from transaction started.
    request->setFlags(0); // 0 = Unicast
    request->setYiaddr(IPv4Address("0.0.0.0")); // NO your IP addr.
    request->setGiaddr(IPv4Address("0.0.0.0")); // NO DHCP Gateway Agents
    request->setChaddr(client_mac_address); // my mac address;
    request->setSname(""); // no server name given
    request->setFile(""); // no file given
    request->getOptions().set(DHCP_MSG_TYPE, DHCPREQUEST);
    request->getOptions().set(CLIENT_ID, "Ethernet:" + client_mac_address.str());

    // set the parameters to request
    request->getOptions().add(PARAM_LIST, SUBNET_MASK);
    request->getOptions().add(PARAM_LIST, ROUTER);
    request->getOptions().add(PARAM_LIST, DNS);
    request->getOptions().add(PARAM_LIST, NTP_SRV);

    if (client_state == REQUESTING)
    {
        // the request is in response of a offer
        request->getOptions().set(SERVER_ID, lease->server_id.str());
        request->getOptions().set(REQUESTED_IP, lease->ip.str());
        request->setCiaddr(IPv4Address("0.0.0.0")); // NO client IP addr.
        EV << "Sending DHCPREQUEST asking for IP " << lease->ip << " via broadcast" << endl;
        sendToUDP(request, bootpc_port, IPv4Address::ALLONES_ADDRESS, bootps_port);
    }
    else if (client_state == RENEWING)
    {
        // the request is for extending the lease
        request->setCiaddr(lease->ip); // the client IP
        EV
                << "Sending DHCPREQUEST extending lease for IP " << lease->ip << " via unicast to "
                        << lease->server_id << endl;
        sendToUDP(request, bootpc_port, lease->server_id, bootps_port);
    }
    else if (client_state == REBINDING)
    {
        // the request is for extending the lease
        request->setCiaddr(lease->ip); // the client IP
        EV << "Sending DHCPREQUEST renewing the IP " << lease->ip << " via broadcast " << endl;
        sendToUDP(request, bootpc_port, IPv4Address::ALLONES_ADDRESS, bootps_port);
    }
}

void DHCPClient::sendDiscover()
{

    // setting the xid
    xid = intuniform(0, RAND_MAX);

    DHCPMessage* discover = new DHCPMessage("DHCPDISCOVER");
    discover->setOp(BOOTREQUEST);
    discover->setByteLength(280); // DHCP Discover packet size;
    discover->setHtype(1); // Ethernet
    discover->setHlen(6); // Hardware Address lenght (6 octets)
    discover->setHops(0);
    discover->setXid(xid); // transacction id;
    discover->setSecs(0); // 0 seconds from transaction started.
    discover->setFlags(0); // 0 = Unicast
    discover->setCiaddr(IPv4Address("0.0.0.0")); // NO client IP addr.
    discover->setYiaddr(IPv4Address("0.0.0.0")); // NO your IP addr.
    discover->setGiaddr(IPv4Address("0.0.0.0")); // NO DHCP Gateway Agents
    discover->setChaddr(client_mac_address); // my mac address;
    discover->setSname(""); // no server name given
    discover->setFile(""); // no file given
    discover->getOptions().set(DHCP_MSG_TYPE, DHCPDISCOVER);
    discover->getOptions().set(CLIENT_ID, "Ethernet:" + client_mac_address.str());
    discover->getOptions().set(REQUESTED_IP, "0.0.0.0");

    // set the parameters to request
    discover->getOptions().add(PARAM_LIST, SUBNET_MASK);
    discover->getOptions().add(PARAM_LIST, ROUTER);
    discover->getOptions().add(PARAM_LIST, DNS);
    discover->getOptions().add(PARAM_LIST, NTP_SRV);

    EV << "Sending DHCPDISCOVER" << endl;
    sendToUDP(discover, bootpc_port, IPv4Address::ALLONES_ADDRESS, bootps_port);
}

void DHCPClient::cancelTimer(cMessage* timer)
{
    // check if the timer exist
    if (timer != NULL)
    {
        cancelEvent(timer);
        delete timer;
    }
}

void DHCPClient::cancelTimer_T1()
{
    cancelTimer(timer_t1);
    timer_t1 = NULL;
}
void DHCPClient::cancelTimer_T2()
{
    cancelTimer(timer_t2);
    timer_t2 = NULL;
}
void DHCPClient::cancelTimer_TO()
{
    cancelTimer(timer_to);
    timer_to = NULL;
}

void DHCPClient::scheduleTimer_TO(TIMER_TYPE type)
{
    // cancel the previous TO
    cancelTimer_TO();
    timer_to = new cMessage("DHCP timeout", type);
    scheduleAt(simTime() + response_timeout, timer_to);
}

void DHCPClient::scheduleTimer_T1()
{
    // cancel the previous T1
    cancelTimer_T1();
    timer_t1 = new cMessage("DHCP T1", T1);
    scheduleAt(simTime() + (lease->renewal_time), timer_t1); // RFC 2131 4.4.5
}
void DHCPClient::scheduleTimer_T2()
{
    // cancel the previous T2
    cancelTimer_T2();
    timer_t2 = new cMessage("DHCP T2", T2);
    scheduleAt(simTime() + (lease->rebind_time), timer_t2); // RFC 2131 4.4.5
}

// Overwrite the sendToUDP in order to add the interface to use to allow the packet be routed by the IP stack
void DHCPClient::sendToUDP(cPacket *msg, int srcPort, const Address& destAddr, int destPort)
{

    EV << "Sending packet: ";
    // printPacket(msg);

   // emit(sentPkSignal, msg);
    UDPSocket::SendOptions options;
    options.outInterfaceId = ie->getInterfaceId();
    socket.sendTo(msg, destAddr, destPort, &options);
}

bool DHCPClient::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if (stage == NodeStartOperation::STAGE_APPLICATION_LAYER) {
            IInterfaceTable* ift = check_and_cast<IInterfaceTable*>(getModuleByPath(par("interfaceTablePath")));
            ie = ift->getInterfaceByName(par("interface"));
            socket.bind(bootpc_port);
            changeFSMState(INIT);
        }
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if (stage == NodeShutdownOperation::STAGE_APPLICATION_LAYER) {
            cancelTimer_T1();
            cancelTimer_T2();
            cancelTimer_TO();
            // TODO: socket.close();
            ie = NULL;
        }
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if (stage == NodeCrashOperation::STAGE_CRASH) {
            cancelTimer_T1();
            cancelTimer_T2();
            cancelTimer_TO();
            ie = NULL;
        }
    }
    else throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

