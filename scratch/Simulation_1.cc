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
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/netanim-module.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/point-to-point-module.h"
#include "stdio.h"
#include "inttypes.h"

//                     Network Topology :
//
//           n1                                          
//           |                       10.0.2.0 
//           |                 
//   n2------n0------n4      n5     n6-------n7     n8     n9
//           |        |      |      |        |      |      |
//           |        ===============        ===============  
//           n3
//           
//       10.0.1.0     LAN 192.118.1.0        LAN 192.118.2.0 

using namespace ns3;


int main(int argc, char* argv[]){

//  Configuration that can be set dynamically from the command line
    CommandLine cmd;
    int configuration = 0;
    cmd.AddValue("configuration", "numero configurazione", configuration);
    cmd.Parse (argc, argv);  

    Time::SetResolution(Time::NS);                                                 
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);                  
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

//  Creating the nodes that make up the star, excluding the central node n0
    uint32_t nSpokes = 4;

//  Creating the nodes that make up the first LAN, excluding n4 since it is already considered in the star and n6 since it will be created with a point to point connection
    uint32_t nCsma1 = 1;
    nCsma1 = nCsma1 == 0 ? 1 : nCsma1;

//  Creating the nodes that make up the second LAN, excluding n7 being already considered in the point-to-point connection
    uint32_t nCsma2 = 2;
    nCsma2 = nCsma2 == 0 ? 1 : nCsma2;

//  Configuring the parameters of the star n0-n{1,2,3,4} and the point-to-point connection between n6 and n7
    NS_LOG_INFO("Build star topology.");
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10us"));
    
//  Creating the star
    PointToPointStarHelper star(nSpokes, pointToPoint);

//  Creating the first LAN starting from n4 also adding n6 which is part of the point-to-point connection
    NodeContainer csmaNodes1;
    csmaNodes1.Add(star.GetSpokeNode(3));
    csmaNodes1.Create(nCsma1);

//  Creating nodes n6 and n7 which will establish a point-to-point connection
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

//  Adding to the first LAN n6, being part of the point-to-point connection
    csmaNodes1.Add(p2pNodes.Get(0));

//  Configuring the parameters of the first LAN
    CsmaHelper csma1;
    csma1.SetChannelAttribute("DataRate", StringValue("25Mbps"));
    csma1.SetChannelAttribute("Delay", StringValue("10us"));

//  Installing the Csma on nodes csmaNodes1
    NetDeviceContainer csmaDevices1;
    csmaDevices1 = csma1.Install(csmaNodes1);

//  Creating the second LAN starting from n7
    NodeContainer csmaNodes2;
    csmaNodes2.Add(p2pNodes.Get(1));
    csmaNodes2.Create(nCsma2);

//  Installing a point-to-point net device on nodes n6 and n7 and a point-to-point channel between them       
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);  

//  Configuring the parameters of the second LAN
    CsmaHelper csma2;
    csma2.SetChannelAttribute("DataRate", StringValue("30Mbps"));
    csma2.SetChannelAttribute("Delay", StringValue("20us"));

//  Installing the Csma on nodes csmaNodes2
    NetDeviceContainer csmaDevices2;
    csmaDevices2 = csma2.Install(csmaNodes2);

//  Installing an Internet Stack (TCP,UDP,IP, etc..) on each of the nodes
    NS_LOG_INFO("Install internet stack on all nodes.");
    InternetStackHelper stack;
    star.InstallStack(stack);
    stack.Install(csmaNodes1.Get(1));
    stack.Install(p2pNodes);
    stack.Install(csmaNodes2.Get(1));
    stack.Install(csmaNodes2.Get(2));

//  Associating the IP addresses to the devices starting from the network 10.0.1.0 for the star with mask 28, 192.118.1.0 for the first LAN with mask 24,
//  192.118.2.0 for the second LAN with mask 24 and 10.0.2.0 for the p2p with mask 30
    NS_LOG_INFO("Assign IP Addresses.");
    star.AssignIpv4Addresses(Ipv4AddressHelper("10.0.1.0", "/28"));

    Ipv4AddressHelper address;
    address.SetBase("192.118.1.0", "/24");
    Ipv4InterfaceContainer csmaInterfaces1 = address.Assign(csmaDevices1);

    address.SetBase("10.0.2.0", "/30");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("192.118.2.0", "/24");
    Ipv4InterfaceContainer csmaInterfaces2 = address.Assign(csmaDevices2);

//  Start configuration 0 :   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( configuration == 0 ) {
        NS_LOG_INFO("Create applications.");

    //  Creation of a packet sink on star node n1, to receive packets
        uint16_t port = 2600;
        Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
        ApplicationContainer sinkApp = sinkHelper.Install(star.GetSpokeNode(0));
        sinkApp.Start(Seconds(1.0));
        sinkApp.Stop(Seconds(20.0));

    //  Creating an OnOff application to send TCP to node n1
        OnOffHelper clientHelper("ns3::TcpSocketFactory", InetSocketAddress(star.GetSpokeIpv4Address(0), port));
        clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize = 1500;
        clientHelper.SetAttribute("PacketSize", UintegerValue(packetSize));

    //  Setting On Off on node n 9
        ApplicationContainer clientApps = clientHelper.Install(csmaNodes2.Get(2));
        AddressValue remoteAddress(InetSocketAddress(csmaInterfaces2.GetAddress(2), port));
        clientHelper.SetAttribute("Remote", remoteAddress);
        clientApps.Start(Seconds(3.0));
        clientApps.Stop(Seconds(15.0));

        NS_LOG_INFO("Enable static global routing.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    //  Capture the packages and create a .pcap file
        NS_LOG_INFO("Enable pcap tracing.");

    //  TEST PCAP on nodes n0, n5, n7
        pointToPoint.EnablePcap("Simulation_1-0-n0.pcap", star.GetHub()->GetDevice(0), true, true);
        csma1.EnablePcap("Simulation_1-0-n5.pcap",csmaDevices1.Get(1),true,true);
        pointToPoint.EnablePcap("Simulation_1-0-n7.pcap",p2pDevices.Get(1),true,true);

    //  ASCII Tracing server n1 client n9  
        AsciiTraceHelper ascii;
        pointToPoint.EnableAscii(ascii.CreateFileStream("Simulation_1-0-n1.tr"),star.GetSpokeNode(0)->GetDevice(0));       
        csma2.EnableAscii(ascii.CreateFileStream("Simulation_1-0-n9.tr"),csmaDevices2.Get(2));
    }

/// Start configuration 1 :  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if ( configuration == 1 ) {

    //  Creation of a packet sink on star node n1, to receive packets
        uint16_t port1 = 2600;
        Address sinkLocalAddress1(InetSocketAddress(Ipv4Address::GetAny(), port1));
        PacketSinkHelper sinkHelper1("ns3::TcpSocketFactory", sinkLocalAddress1);
        ApplicationContainer sinkApp1 = sinkHelper1.Install(star.GetSpokeNode(0));
        sinkApp1.Start(Seconds(1.0));
        sinkApp1.Stop(Seconds(20.0));

    //  Creating an OnOff application to send TCP to node n1
        OnOffHelper clientHelper1("ns3::TcpSocketFactory", InetSocketAddress(star.GetSpokeIpv4Address(0), port1));
        clientHelper1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize1 = 2500;
        clientHelper1.SetAttribute("PacketSize", UintegerValue(packetSize1));

    //  Setting On Off on node n 9
        ApplicationContainer clientApps1 = clientHelper1.Install(csmaNodes2.Get(2));
        AddressValue remoteAddress1(InetSocketAddress(csmaInterfaces2.GetAddress(2) , port1));
        clientHelper1.SetAttribute("Remote", remoteAddress1);
        clientApps1.Start(Seconds(5.0));
        clientApps1.Stop(Seconds(15.0));

    //  Creation of a packet sink on star node n2, to receive packets
        uint16_t port2 = 7777;
        Address sinkLocalAddress2(InetSocketAddress(Ipv4Address::GetAny(), port2));
        PacketSinkHelper sinkHelper2("ns3::TcpSocketFactory", sinkLocalAddress2);
        ApplicationContainer sinkApp2 = sinkHelper2.Install(star.GetSpokeNode(1));
        sinkApp2.Start(Seconds(1.0));
        sinkApp2.Stop(Seconds(20.0));

    //  Creating an OnOff application to send TCP to node n2
        OnOffHelper clientHelper2("ns3::TcpSocketFactory", InetSocketAddress(star.GetSpokeIpv4Address(1), port2));
        clientHelper2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize2 = 5000;
        clientHelper2.SetAttribute("PacketSize", UintegerValue(packetSize2));

    //  Setting of OnOff on node n8
        ApplicationContainer clientApps2 = clientHelper2.Install(csmaNodes2.Get(1)) ;
        AddressValue remoteAddress2(InetSocketAddress(csmaInterfaces2.GetAddress(1), port2));
        clientHelper2.SetAttribute("Remote", remoteAddress2);
        clientApps2.Start(Seconds(2.0));
        clientApps2.Stop(Seconds(9.0));

        NS_LOG_INFO("Enable static global routing.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    //  Capture the packages and create a .pcap file
        NS_LOG_INFO("Enable pcap tracing.");

    //  TEST PCAP on nodes n0, n5, n7
        pointToPoint.EnablePcap("Simulation_1-1-n0.pcap", star.GetHub()->GetDevice(0), true, true);
        csma1.EnablePcap("Simulation_1-1-n5.pcap",csmaDevices1.Get(1),true,true);
        pointToPoint.EnablePcap("Simulation_1-1-n7.pcap",p2pDevices.Get(1),true,true);

    //  ASCII Tracing on n1 n2 servers and n8 n9 clients   
        AsciiTraceHelper ascii;
        pointToPoint.EnableAscii(ascii.CreateFileStream("Simulation_1-1-n1.tr"),star.GetSpokeNode(0)->GetDevice(0));       
        pointToPoint.EnableAscii(ascii.CreateFileStream("Simulation_1-1-n2.tr"),star.GetSpokeNode(1)->GetDevice(0));     
        csma2.EnableAscii(ascii.CreateFileStream("Simulation_1-1-n8.tr"),csmaDevices2.Get(1));
        csma2.EnableAscii(ascii.CreateFileStream("Simulation_1-1-n9.tr"),csmaDevices2.Get(2));
    }
    
/// Start configuration 2 :  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //  UDP Echo Server on n2, port 63   
        UdpEchoServerHelper echoServer(63);

        ApplicationContainer serverApps = echoServer.Install(star.GetSpokeNode(1));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(20.0));

    //  UDP Echo Client on n8
        uint32_t maxPacketCount = 5;
        Time interPacketInterval = Seconds(2.);
        UdpEchoClientHelper echoClient(star.GetSpokeIpv4Address(1), 63);
        echoClient.SetAttribute("PacketSize", UintegerValue(2560));
        echoClient.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        echoClient.SetAttribute("Interval", TimeValue(interPacketInterval));

        ApplicationContainer clientApps = echoClient.Install(csmaNodes2.Get(1));
        clientApps.Start(Seconds(3.0));
        clientApps.Stop(Seconds(15.0));

    //  TCP Sink on n1, port 2600
        uint16_t port3 = 2600;
        Address sinkLocalAddress3(InetSocketAddress(Ipv4Address::GetAny(), port3));
        PacketSinkHelper sinkHelper3("ns3::TcpSocketFactory", sinkLocalAddress3);
        ApplicationContainer sinkApp3 = sinkHelper3.Install(star.GetSpokeNode(0));
        sinkApp3.Start(Seconds(1.0));
        sinkApp3.Stop(Seconds(20.0));

    //  TCP OnOff Client n9
        OnOffHelper clientHelper3("ns3::TcpSocketFactory", InetSocketAddress(star.GetSpokeIpv4Address(0), port3));
        clientHelper3.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper3.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize3 = 3000;
        clientHelper3.SetAttribute("PacketSize", UintegerValue(packetSize3));

    //  Setting On Off on node n 9
        ApplicationContainer clientApps3 = clientHelper3.Install(csmaNodes2.Get(2));
        AddressValue remoteAddress3(InetSocketAddress(csmaInterfaces2.GetAddress(2) , port3));
        clientHelper3.SetAttribute("Remote", remoteAddress3);
        clientApps3.Start(Seconds(3.0));
        clientApps3.Stop(Seconds(9.0));

    //  UDP Sink on n3, port 2500
        uint16_t port4 = 2500;
        Address sinkLocalAddress4(InetSocketAddress(Ipv4Address::GetAny(), port4));
        PacketSinkHelper sinkHelper4("ns3::UdpSocketFactory", sinkLocalAddress4);
        ApplicationContainer sinkApp4 = sinkHelper4.Install(star.GetSpokeNode(2));
        sinkApp4.Start(Seconds(1.0));
        sinkApp4.Stop(Seconds(20.0));

    //  UDP OnOff Client n8
        OnOffHelper clientHelper4("ns3::UdpSocketFactory", InetSocketAddress(star.GetSpokeIpv4Address(2), port4));
        clientHelper4.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper4.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize4 = 3000;
        clientHelper4.SetAttribute("PacketSize", UintegerValue(packetSize4));

    //  Setting of OnOff on node n8
        ApplicationContainer clientApps4 = clientHelper4.Install(csmaNodes2.Get(1));
        AddressValue remoteAddress4(InetSocketAddress(csmaInterfaces2.GetAddress(1) , port4));
        clientHelper4.SetAttribute("Remote", remoteAddress4);
        clientApps4.Start(Seconds(5.0));
        clientApps4.Stop(Seconds(15.0));
    
    //  Setting the message in the UDP packets
        std::string matricola = " message ";
        int size = 2559 - matricola.size();
        for (int i = 0; i < size; i++){
            matricola += char(0);
        }
        echoClient.SetFill(clientApps.Get(0), matricola);

        NS_LOG_INFO("Enable static global routing.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    //  Capture the packages and create a .pcap file
        NS_LOG_INFO("Enable pcap tracing.");

    //  TEST PCAP on nodes n0, n5, n7
        pointToPoint.EnablePcap("Simulation_1-2-n0.pcap", star.GetHub()->GetDevice(0), true, true);
        csma1.EnablePcap("Simulation_1-2-n5.pcap",csmaDevices1.Get(1),true,true);
        pointToPoint.EnablePcap("Simulation_1-2-n7.pcap",p2pDevices.Get(1),true,true);

    //  ASCII Tracing on server n1 n2 n3 and client n8 n9
        AsciiTraceHelper ascii;
        pointToPoint.EnableAscii(ascii.CreateFileStream("Simulation_1-2-n1.tr"),star.GetSpokeNode(0)->GetDevice(0));       
        pointToPoint.EnableAscii(ascii.CreateFileStream("Simulation_1-2-n2.tr"),star.GetSpokeNode(1)->GetDevice(0));     
        pointToPoint.EnableAscii(ascii.CreateFileStream("Simulation_1-2-n3.tr"),star.GetSpokeNode(2)->GetDevice(0));       
        csma2.EnableAscii(ascii.CreateFileStream("Simulation_1-2-n8.tr"),csmaDevices2.Get(1));
        csma2.EnableAscii(ascii.CreateFileStream("Simulation_1-2-n9.tr"),csmaDevices2.Get(2));
    }

/// Printing IP addresses of the nodes  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::ostream& os = std::cout;
    printf("Node: %" PRIu32" ",star.GetHub()->GetId());
    star.GetHubIpv4Address(0).Print(os);
    puts("\n");
    printf("Node: %" PRIu32" ",star.GetSpokeNode(0)->GetId());
    star.GetSpokeIpv4Address(0).Print(os);
    puts("\n");
    printf("Node: %" PRIu32" ",star.GetSpokeNode(1)->GetId());
    star.GetSpokeIpv4Address(1).Print(os);
    puts("\n");
    printf("Node: %" PRIu32" ",star.GetSpokeNode(2)->GetId());
    star.GetSpokeIpv4Address(2).Print(os);
    puts("\n");

    for (uint32_t i = 0; i<csmaNodes1.GetN();i++){
        printf("Node: %" PRIu32" ",csmaNodes1.Get(i)->GetId());
        csmaInterfaces1.Get(i).first->GetAddress(1,0).GetLocal().Print(os);
        puts("\n");
    }

    for (uint32_t i = 0; i<csmaNodes2.GetN();i++){
        printf("Node: %" PRIu32" ",csmaNodes2.Get(i)->GetId());
        csmaInterfaces2.Get(i).first->GetAddress(1,0).GetLocal().Print(os);
        puts("\n");
    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
