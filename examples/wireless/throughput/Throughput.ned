//
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


package inet.examples.wireless.throughput;

import inet.mobility.models.LinearMobility;
import inet.world.radio.ChannelControl;
//import inet.mobility.models.StationaryMobility;
import inet.linklayer.ieee80211.Ieee80211Nic;
import inet.base.NotificationBoard;
import inet.applications.ethernet.EtherAppCli;


module ThroughputClient
{
    parameters:
        @display("i=device/wifilaptop");
        @node();
    gates:
        input radioIn @directIn;

    submodules:
        notificationBoard: NotificationBoard {
            parameters:
                @display("p=52,70");
        }
        cli: EtherAppCli {
            parameters:
                registerSAP = false;
                @display("b=40,24;p=180,60,col");
        }
        wlan: Ieee80211Nic {
            parameters:
                @display("p=112,134;q=queue");
                mgmtType = default("Ieee80211MgmtSTASimplified");
        }
        mobility: LinearMobility {
            parameters:
                @display("p=50,141");
                constraintAreaMinX = default(-1m);
        }
    connections allowunconnected:
        wlan.radioIn <-- radioIn;
        cli.out --> wlan.upperLayerIn;
}

network Throughput
{
    parameters:
        int numCli;
        @display("b=297,203;bgb=961,248");
    submodules:
        cliHost[numCli]: ThroughputClient {
            parameters:
                @display("r=200,,#707070;p=535,104,,5");
        }
        ap: WirelessAPWithSink {
            parameters:
                @display("p=482,210;r=,,#707070");
        }
        channelControl: ChannelControl {
            parameters:
                @display("p=61,46");
        }
}

