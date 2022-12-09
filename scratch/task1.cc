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
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("HW2_Task1_Team_48");

int main(int argc, char* argv[])
{
    bool verbose = true;
    // Setting the number of wifi nodes
    uint32_t nWifi = 5;
    bool tracing = false;
    bool useRtsCts = false;
    bool useNetAnim = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc, argv);

    /*
    
    PASSARE DA LINEA DI COMANDO?

    bool enableCtsRts = true;
    UintegerValue ctsThreshold = (enableCtsRts? UintegerValue(100): UintegerValue(2346));
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThreshold);
    
    */

    if (verbose)
    {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
    // Creating wifi nodes
    NodeContainer wifiAdHocNodes;
    wifiAdHocNodes.Create(nWifi);

    // Setting the channel and the physic layer
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Setting wifi and AARF algorithm
    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    // Add a mac and set it to adhoc mode
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer adHocDevices = wifi.Install(phy, mac, wifiAdHocNodes);

    // Adding mobility models
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

    // Installing mobility models on the adhoc nodes
    mobility.Install(wifiAdHocNodes);

    // Installing stack and assigning IP addresses to our device interface
    InternetStackHelper stack;
    stack.Install(wifiAdHocNodes);

    Ipv4AddressHelper address;
    NS_LOG_INFO("Assign IP Addresses.");
    address.SetBase("192.168.1.0", "/24");
    Ipv4InterfaceContainer adHocInterface = address.Assign(adHocDevices);
    
    
//  UDP Echo Server on n0, port 20
    UdpEchoServerHelper echoServer(20);
    ApplicationContainer serverApps = echoServer.Install(wifiAdHocNodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

//  UDP Echo Client on n4
    UdpEchoClientHelper echoClient(adHocInterface.GetAddress(0), 20);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApps = echoClient.Install(wifiAdHocNodes.Get(3));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(2.0));
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  /*  
    if (tracing)
    {
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.EnablePcap("third", apDevices.Get(0));
    }
    */  
    
    if(useNetAnim) {
        // Creating state parameter with default configuration off
        std::string state = "off";

        if (useRtsCts == true) {
            state = "on";
        }

        AnimationInterface anim(std::string("wireless-task1-rts-") + state + ".xml");                   // rivedere: prof dice di dichiararlo fuori 

        for (uint32_t i = 0; i < wifiAdHocNodes.GetN(); ++i)
        {
            anim.UpdateNodeDescription(wifiAdHocNodes.Get(i), "ADHOC"); 
            anim.UpdateNodeColor(wifiAdHocNodes.Get(i), 255, 0, 0);
        }

        // Enabling writing the packet metadata to the XML trace
        anim.EnablePacketMetadata();

        /*   
    anim.EnableIpv4RouteTracking("routingtable-wireless.xml",
                                 Seconds(0),
                                 Seconds(5),
                                 Seconds(0.25));
        anim.EnableWifiMacCounters(Seconds(0), Seconds(10));
        anim.EnableWifiPhyCounters(Seconds(0), Seconds(10));        */
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        Simulator::Destroy();
    }
    else {
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        Simulator::Destroy();
    }
    return 0;
}       
