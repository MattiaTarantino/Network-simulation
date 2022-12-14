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
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HW2_Task2_Team_48");

int main(int argc, char* argv[]){

//  Setting the number of wifi nodes
    uint32_t nWifi = 5;

//  Setting the number for the Access Point
    uint32_t nAP = 1;

//  Creating state parameter with default configuration off
    std::string state = "off";

//  Setting command line parameters
    bool useRtsCts = false;
    bool verbose = false;
    bool useNetAnim = false;
    Ssid ssid = Ssid("TLC2022");

    CommandLine cmd(__FILE__);
    cmd.AddValue("useRtsCts", "Enable Rts and Cts frames", useRtsCts);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("useNetAnim", "Enable NetAnim", useNetAnim);
    cmd.AddValue("ssid", "Set ssid", ssid);
   
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

//  Creating wifi StaNodes
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);

//  Creating wifi AP node
    NodeContainer wifiApNode;
    wifiApNode.Create(nAP);

//  Setting the channel and the physic layer
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    wifi.SetStandard(WifiStandard(WIFI_STANDARD_80211g));

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer StaDevices = wifi.Install(phy, mac, wifiStaNodes);

    NetDeviceContainer ApDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    ApDevices = wifi.Install(phy, mac, wifiApNode);

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
//  Installing mobility models on the nodes                          
    mobility.Install(wifiStaNodes);

//  Installing mobility models on the Access Point 
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;

    address.SetBase("192.168.1.0", "/24");
    Ipv4InterfaceContainer StaInterface = address.Assign(StaDevices);
    Ipv4InterfaceContainer ApInterface = address.Assign(ApDevices);

//  UDP Echo Server on n0, port 21 
    UdpEchoServerHelper echoServer(21);
    ApplicationContainer serverApps = echoServer.Install(wifiStaNodes.Get(0));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(10.0));

//  UDP Echo Client on n3
    UdpEchoClientHelper echoClient3(StaInterface.GetAddress(0), 21);
    echoClient3.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient3.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient3.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApps = echoClient3.Install(wifiStaNodes.Get(3));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(5.0));

//  UDP Echo Client on n4
    UdpEchoClientHelper echoClient4(StaInterface.GetAddress(0), 21);
    echoClient4.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient4.SetAttribute("Interval", TimeValue(Seconds(3.0)));
    echoClient4.SetAttribute("PacketSize", UintegerValue(512));

    clientApps = echoClient4.Install(wifiStaNodes.Get(4));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(5.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(7.0));

//  Tracing
    NodeList list;
    std::string s1 = std::to_string(list.GetNode(4) -> GetId());
    std::string s2 = std::to_string(list.GetNode(5) -> GetId());
    phy.EnablePcap("task2-" + state + "-" + s1 + ".pcap", StaDevices.Get(4), true, true);
    phy.EnablePcap("task2-" + state + "-" + s2 + ".pcap", ApDevices.Get(0), true, true);

//  NetAnim
    if(useNetAnim) {
        AnimationInterface anim(std::string("wireless-task2-rts-") + state + ".xml");               

        anim.UpdateNodeDescription(wifiStaNodes.Get(0), "SRV-" + std::to_string(list.GetNode(0) -> GetId()));
        anim.UpdateNodeColor(wifiStaNodes.Get(0), 255, 0, 0);                                     

        anim.UpdateNodeDescription(wifiStaNodes.Get(3), "CLI-" + std::to_string(list.GetNode(3) -> GetId()));
        anim.UpdateNodeColor(wifiStaNodes.Get(3), 0, 255, 0);                                     

        anim.UpdateNodeDescription(wifiStaNodes.Get(4), "CLI-" + std::to_string(list.GetNode(4) -> GetId()));
        anim.UpdateNodeColor(wifiStaNodes.Get(4), 0, 255, 0);                                     

        anim.UpdateNodeDescription(wifiStaNodes.Get(1), "STA-" + std::to_string(list.GetNode(1) -> GetId()));
        anim.UpdateNodeColor(wifiStaNodes.Get(1), 0, 0, 255);                                    

        anim.UpdateNodeDescription(wifiStaNodes.Get(2), "STA-" + std::to_string(list.GetNode(2) -> GetId()));
        anim.UpdateNodeColor(wifiStaNodes.Get(2), 0, 0, 255);        

        anim.UpdateNodeDescription(wifiApNode.Get(0), "AP");
        anim.UpdateNodeColor(wifiApNode.Get(0), 66, 49, 137);                             

    //  Enabling writing the packet metadata to the XML trace
        anim.EnablePacketMetadata();
        anim.EnableIpv4RouteTracking("routingtable-wireless-task2.xml",
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
