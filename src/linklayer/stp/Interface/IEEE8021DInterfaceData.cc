// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 
// Author: Benjamin Martin Seregi

#include "IEEE8021DInterfaceData.h"

// TODO: Currently, it is working with STP but may need to change to work with RSTP

bool IEEE8021DInterfaceData::isLearning()
{
    if (portData.state == LEARNING || portData.state == FORWARDING)
        return true;

    return false;
}

bool IEEE8021DInterfaceData::isForwarding()
{
    if (portData.state == FORWARDING)
        return true;

    return false;
}

unsigned int IEEE8021DInterfaceData::getAge() const
{
    return portData.age;
}

void IEEE8021DInterfaceData::setAge(unsigned int age)
{
    portData.age = age;
}

const MACAddress& IEEE8021DInterfaceData::getBridgeAddress() const
{
    return portData.bridgeAddress;
}

void IEEE8021DInterfaceData::setBridgeAddress(const MACAddress& bridgeAddress)
{
    portData.bridgeAddress = bridgeAddress;
}

unsigned int IEEE8021DInterfaceData::getBridgePriority() const
{
    return portData.bridgePriority;
}

void IEEE8021DInterfaceData::setBridgePriority(unsigned int bridgePriority)
{
    portData.bridgePriority = bridgePriority;
}

unsigned int IEEE8021DInterfaceData::getFdWhile() const
{
    return portData.fdWhile;
}

void IEEE8021DInterfaceData::setFdWhile(unsigned int fdWhile)
{
    portData.fdWhile = fdWhile;
}

unsigned int IEEE8021DInterfaceData::getFwdDelay() const
{
    return portData.fwdDelay;
}

void IEEE8021DInterfaceData::setFwdDelay(unsigned int fwdDelay)
{
    portData.fwdDelay = fwdDelay;
}

unsigned int IEEE8021DInterfaceData::getHelloTime() const
{
    return portData.helloTime;
}

void IEEE8021DInterfaceData::setHelloTime(unsigned int helloTime)
{
    portData.helloTime = helloTime;
}

unsigned int IEEE8021DInterfaceData::getLinkCost() const
{
    return portData.linkCost;
}

void IEEE8021DInterfaceData::setLinkCost(unsigned int linkCost)
{
    portData.linkCost = linkCost;
}

unsigned int IEEE8021DInterfaceData::getMaxAge() const
{
    return portData.maxAge;
}

void IEEE8021DInterfaceData::setMaxAge(unsigned int maxAge)
{
    portData.maxAge = maxAge;
}

unsigned int IEEE8021DInterfaceData::getPortNum() const
{
    return portData.portNum;
}

void IEEE8021DInterfaceData::setPortNum(unsigned int portNum)
{
    portData.portNum = portNum;
}

unsigned int IEEE8021DInterfaceData::getPortPriority() const
{
    return portData.portPriority;
}

void IEEE8021DInterfaceData::setPortPriority(unsigned int portPriority)
{
    portData.portPriority = portPriority;
}

unsigned int IEEE8021DInterfaceData::getPriority() const
{
    return portData.priority;
}

void IEEE8021DInterfaceData::setPriority(unsigned int priority)
{
    portData.priority = priority;
}

IEEE8021DInterfaceData::PortRole IEEE8021DInterfaceData::getRole() const
{
    return portData.role;
}

void IEEE8021DInterfaceData::setRole(PortRole role)
{
    portData.role = role;
}

const MACAddress& IEEE8021DInterfaceData::getRootAddress() const
{
    return portData.rootAddress;
}

void IEEE8021DInterfaceData::setRootAddress(const MACAddress& rootAddress)
{
    portData.rootAddress = rootAddress;
}

unsigned int IEEE8021DInterfaceData::getRootPathCost() const
{
    return portData.rootPathCost;
}

void IEEE8021DInterfaceData::setRootPathCost(unsigned int rootPathCost)
{
    portData.rootPathCost = rootPathCost;
}

unsigned int IEEE8021DInterfaceData::getRootPriority() const
{
    return portData.rootPriority;
}

void IEEE8021DInterfaceData::setRootPriority(unsigned int rootPriority)
{
    portData.rootPriority = rootPriority;
}

IEEE8021DInterfaceData::PortState IEEE8021DInterfaceData::getState() const
{
    return portData.state;
}

void IEEE8021DInterfaceData::setState(PortState state)
{
    portData.state = state;
}

IEEE8021DInterfaceData::PortInfo IEEE8021DInterfaceData::getPortInfoData()
{
    return portData;
}

void IEEE8021DInterfaceData::setPortInfoData(PortInfo& portInfoData)
{
    portData = portInfoData;
}
