/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HW2_Task1_Team_48");

int main(int argc, char* argv[]){

//  Setting the number of wifi nodes
    uint32_t nWifi = 5;

//  Creating state parameter with default configuration off
    std::string state = "off";

//  Setting command line parameters
    bool useRtsCts = false;
    bool verbose = false;
    bool useNetAnim = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("useRtsCts", "Enable Rts and Cts frames", useRtsCts);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("useNetAnim", "Enable NetAnim", useNetAnim);
    cmd.Parse(argc, argv);

//  Forcing the use of RTS and CTS
    UintegerValue ctsThreshold = (useRtsCts? UintegerValue(100): UintegerValue(2346));
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThreshold);

    if (verbose){
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

//  Using RTS and CTS
    if (useRtsCts == true) {
        state = "on";
    }

//  Creating wifi AdHocNodes
    NodeContainer wifiAdHocNodes;
    wifiAdHocNodes.Create(nWifi);

//  Setting the channel and the physic layer
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

//  Setting wifi and AARF algorithm
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    wifi.SetStandard(WifiStandard(WIFI_STANDARD_80211g));

//  Add a mac and set it to adhoc mode
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer adHocDevices;
    adHocDevices = wifi.Install(phy, mac, wifiAdHocNodes);

//  Adding mobility models
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-90, 90, -90, 90)));

//  Installing mobility models on the adhoc nodes
    mobility.Install(wifiAdHocNodes);

//  Installing stack and assigning IP addresses to our device interface
    InternetStackHelper stack;
    stack.Install(wifiAdHocNodes);

    Ipv4AddressHelper address;
    NS_LOG_INFO("Assign IP Addresses.");
    address.SetBase("192.168.1.0", "/24");
    Ipv4InterfaceContainer adHocInterface = address.Assign(adHocDevices);
   
//  UDP Echo Server on n0, port 20 
    UdpEchoServerHelper echoServer(20);
    ApplicationContainer serverApps = echoServer.Install(wifiAdHocNodes.Get(0));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(10.0));

//  UDP Echo Client on n4
    UdpEchoClientHelper echoClient4(adHocInterface.GetAddress(0), 20);
    echoClient4.SetAttribute("MaxPackets", UintegerValue(2));
    echoClient4.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient4.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApps = echoClient4.Install(wifiAdHocNodes.Get(4));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(3.0));

//  UDP Echo Client on n3
    UdpEchoClientHelper echoClient3(adHocInterface.GetAddress(0), 20);
    echoClient3.SetAttribute("MaxPackets", UintegerValue(2));
    echoClient3.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient3.SetAttribute("PacketSize", UintegerValue(512));

    clientApps = echoClient3.Install(wifiAdHocNodes.Get(3));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(5.0));
   
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(7.0));

//  Tracing
    NodeList list;
    std::string s = std::to_string(list.GetNode(2) -> GetId());
    phy.EnablePcap("task1-" + state + "-" + s + ".pcap", adHocDevices.Get(2), true, true);

//  NetAnim
    if(useNetAnim) {
        AnimationInterface anim(std::string("wireless-task1-rts-") + state + ".xml");               

        anim.UpdateNodeDescription(wifiAdHocNodes.Get(0), "SRV-0");
        anim.UpdateNodeColor(wifiAdHocNodes.Get(0), 255, 0, 0);                                     // rivedere id

        anim.UpdateNodeDescription(wifiAdHocNodes.Get(3), "CLI-3");
        anim.UpdateNodeColor(wifiAdHocNodes.Get(3), 0, 255, 0);                                     // rivedere id

        anim.UpdateNodeDescription(wifiAdHocNodes.Get(4), "CLI-4");
        anim.UpdateNodeColor(wifiAdHocNodes.Get(4), 0, 255, 0);                                     // rivedere id

        anim.UpdateNodeDescription(wifiAdHocNodes.Get(1), "HOC-1");
        anim.UpdateNodeColor(wifiAdHocNodes.Get(1), 0, 0, 255);                                     // rivedere id

        anim.UpdateNodeDescription(wifiAdHocNodes.Get(2), "HOC-2");
        anim.UpdateNodeColor(wifiAdHocNodes.Get(2), 0, 0, 255);                                     // rivedere id

    //  Enabling writing the packet metadata to the XML trace
        anim.EnablePacketMetadata();
        anim.EnableIpv4RouteTracking("routingtable-wireless-task1.xml",
                                 Seconds(0),
                                 Seconds(10),
                                 Seconds(0.25));                                                        
        anim.EnableWifiMacCounters(Seconds(0), Seconds(10));
        anim.EnableWifiPhyCounters(Seconds(0), Seconds(10));
        Simulator::Run();
        Simulator::Destroy();
    }
    else {
        Simulator::Run();
        Simulator::Destroy();
    }
    return 0;
}