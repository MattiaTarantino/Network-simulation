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

NS_LOG_COMPONENT_DEFINE("Task_1_Team_48");

int main(int argc, char* argv[]){

//  Configurazione impostabile dinamicamente da linea di comando
    CommandLine cmd;
    int configuration = 1;
    cmd.AddValue("configuration", "numero configurazione", configuration);
    cmd.Parse (argc, argv);  
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);                  
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    Time::SetResolution(Time::NS);                                                 
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

//  Creando i nodi che compongono la stella, escluso il nodo centrale n0
    uint32_t nSpokes = 4;

//  Creando i nodi che compongono la prima LAN, escluso n4 essendo già considerato nella stella e n6 dato che verrà creato con una connessione point to point
    uint32_t nCsma1 = 1;
    nCsma1 = nCsma1 == 0 ? 1 : nCsma1;

//  Creando i nodi che compongono la seconda LAN, escluso n7 essendo già considerato nella connessione point-to-point
    uint32_t nCsma2 = 2;
    nCsma2 = nCsma2 == 0 ? 1 : nCsma2;

//  Configurando i parametri della stella n0-n{1,2,3,4} e della connessione point-to-point tra n6 e n7
    NS_LOG_INFO("Build star topology.");
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10us"));
    
//  Creando la stella
    PointToPointStarHelper star(nSpokes, pointToPoint);

//  Creando la prima LAN partendo da n4 aggiungendo anche n6 facente parte della connessione point-to-point 
    NodeContainer csmaNodes1;
    csmaNodes1.Add(star.GetSpokeNode(3));
    csmaNodes1.Create(nCsma1);

//  Creando i nodi n6 e n7 che andranno a instaurare una connessione point-to-point
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

//  Aggiungendo alla prima LAN n6, facente parte della connessione point-to-point
    csmaNodes1.Add(p2pNodes.Get(0));

//  Configurando i parametri della prima LAN
    CsmaHelper csma1;
    csma1.SetChannelAttribute("DataRate", StringValue("25Mbps"));
    csma1.SetChannelAttribute("Delay", StringValue("10us"));

//  Installando la Csma sui nodi csmaNodes1
    NetDeviceContainer csmaDevices1;
    csmaDevices1 = csma1.Install(csmaNodes1);

//  Creando la seconda LAN partendo da n7
    NodeContainer csmaNodes2;
    csmaNodes2.Add(p2pNodes.Get(1));
    csmaNodes2.Create(nCsma2);

//  Installando un point-to-point net device sui nodi n6 e n7 e un canale point-to-point tra essi        
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);  

//  Configurando i parametri della seconda LAN
    CsmaHelper csma2;
    csma2.SetChannelAttribute("DataRate", StringValue("30Mbps"));
    csma2.SetChannelAttribute("Delay", StringValue("20us"));

//  Installando la Csma sui nodi csmaNodes2
    NetDeviceContainer csmaDevices2;
    csmaDevices2 = csma2.Install(csmaNodes2);

//  Installando una Stack Internet (TCO,UDP,IP, ecc..) su ognuno dei nodi
    NS_LOG_INFO("Install internet stack on all nodes.");
    InternetStackHelper stack;
    star.InstallStack(stack);
    stack.Install(csmaNodes1.Get(1));
    stack.Install(p2pNodes);
    stack.Install(csmaNodes2.Get(1));
    stack.Install(csmaNodes2.Get(2));

//  Associando gli indirizzi IP ai devices partendo dal network 10.0.1.0 per la stella con maschera 28, 192.118.1.0 per la prima LAN con maschera 24,
//  192.118.2.0 per la seconda LAN con maschera 24 e 10.0.2.0 per la p2p con maschera 30
    NS_LOG_INFO("Assign IP Addresses.");
    star.AssignIpv4Addresses(Ipv4AddressHelper("10.0.1.0", "/28"));

    Ipv4AddressHelper address;
    address.SetBase("192.118.1.0", "/24");
    Ipv4InterfaceContainer csmaInterfaces1 = address.Assign(csmaDevices1);

    address.SetBase("10.0.2.0", "/30");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("192.118.2.0", "/24");
    Ipv4InterfaceContainer csmaInterfaces2 = address.Assign(csmaDevices2);

//  Inizio configurazione 0 :   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( configuration == 0 ) {
        NS_LOG_INFO("Create applications.");

    //  Creazione di un packet sink sul nodo n1 della stella, per ricevere i pacchetti
        uint16_t port = 2600;
        Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
        ApplicationContainer sinkApp = sinkHelper.Install(star.GetSpokeNode(0));
        sinkApp.Start(Seconds(1.0));
        sinkApp.Stop(Seconds(20.0));

    //  Creazione di un' applicazioni OnOff per mandare TCP al nodo n1
        OnOffHelper clientHelper("ns3::TcpSocketFactory", InetSocketAddress(star.GetSpokeIpv4Address(0), port));
        clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize = 1500;
        clientHelper.SetAttribute("PacketSize", UintegerValue(packetSize));

    //  Impostazione dell' OnOff sul nodo n9
        ApplicationContainer clientApps = clientHelper.Install(csmaNodes2.Get(2));
        AddressValue remoteAddress(InetSocketAddress(csmaInterfaces2.GetAddress(2), port));
        clientHelper.SetAttribute("Remote", remoteAddress);
        clientApps.Start(Seconds(3.0));
        clientApps.Stop(Seconds(15.0));

        NS_LOG_INFO("Enable static global routing.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    //  Cattura i pacchetti e crea un file .pcap
        NS_LOG_INFO("Enable pcap tracing.");

    //  TEST PCAP sui nodi n0, n5, n7
        pointToPoint.EnablePcap("task1-0-n0.pcap", star.GetHub()->GetDevice(0), true, true);
        csma1.EnablePcap("task1-0-n5.pcap",csmaDevices1.Get(1),true,true);
        pointToPoint.EnablePcap("task1-0-n7.pcap",p2pDevices.Get(1),true,true);

    //  ASCII Tracing server n1 client n9  
        AsciiTraceHelper ascii;
        pointToPoint.EnableAscii(ascii.CreateFileStream("task1-0-n1.tr"),star.GetSpokeNode(0)->GetDevice(0));       
        csma2.EnableAscii(ascii.CreateFileStream("task1-0-n9.tr"),csmaDevices2.Get(2));
    }

/// Inizio configurazione 1 :  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if ( configuration == 1 ) {

    //  Creazione di un packet sink sul nodo n1 della stella, per ricevere i pacchetti
        uint16_t port1 = 2600;
        Address sinkLocalAddress1(InetSocketAddress(Ipv4Address::GetAny(), port1));
        PacketSinkHelper sinkHelper1("ns3::TcpSocketFactory", sinkLocalAddress1);
        ApplicationContainer sinkApp1 = sinkHelper1.Install(star.GetSpokeNode(0));
        sinkApp1.Start(Seconds(1.0));
        sinkApp1.Stop(Seconds(20.0));

    //  Creazione di un' applicazioni OnOff per mandare TCP al nodo n1
        OnOffHelper clientHelper1("ns3::TcpSocketFactory", InetSocketAddress(star.GetSpokeIpv4Address(0), port1));
        clientHelper1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize1 = 2500;
        clientHelper1.SetAttribute("PacketSize", UintegerValue(packetSize1));

    //  Impostazione dell' OnOff sul nodo n9
        ApplicationContainer clientApps1 = clientHelper1.Install(csmaNodes2.Get(2));
        AddressValue remoteAddress1(InetSocketAddress(csmaInterfaces2.GetAddress(2) , port1));
        clientHelper1.SetAttribute("Remote", remoteAddress1);
        clientApps1.Start(Seconds(5.0));
        clientApps1.Stop(Seconds(15.0));

    //  Creazione di un packet sink sul nodo n2 della stella, per ricevere i pacchetti
        uint16_t port2 = 7777;
        Address sinkLocalAddress2(InetSocketAddress(Ipv4Address::GetAny(), port2));
        PacketSinkHelper sinkHelper2("ns3::TcpSocketFactory", sinkLocalAddress2);
        ApplicationContainer sinkApp2 = sinkHelper2.Install(star.GetSpokeNode(1));
        sinkApp2.Start(Seconds(1.0));
        sinkApp2.Stop(Seconds(20.0));

    //  Creazione di un' applicazioni OnOff per mandare TCP al nodo n2
        OnOffHelper clientHelper2("ns3::TcpSocketFactory", InetSocketAddress(star.GetSpokeIpv4Address(1), port2));
        clientHelper2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize2 = 5000;
        clientHelper2.SetAttribute("PacketSize", UintegerValue(packetSize2));

    //  Impostazione dell' OnOff sul nodo n8
        ApplicationContainer clientApps2 = clientHelper2.Install(csmaNodes2.Get(1)) ;
        AddressValue remoteAddress2(InetSocketAddress(csmaInterfaces2.GetAddress(1), port2));
        clientHelper2.SetAttribute("Remote", remoteAddress2);
        clientApps2.Start(Seconds(2.0));
        clientApps2.Stop(Seconds(9.0));

        NS_LOG_INFO("Enable static global routing.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    //  Cattura i pacchetti e crea un file .pcap
        NS_LOG_INFO("Enable pcap tracing.");

    //  TEST PCAP sui nodi n0, n5, n7
        pointToPoint.EnablePcap("task1-1-n0.pcap", star.GetHub()->GetDevice(0), true, true);
        csma1.EnablePcap("task1-1-n5.pcap",csmaDevices1.Get(1),true,true);
        pointToPoint.EnablePcap("task1-1-n7.pcap",p2pDevices.Get(1),true,true);

    //  ASCII Tracing su server n1 n2 e client n8 n9   
        AsciiTraceHelper ascii;
        pointToPoint.EnableAscii(ascii.CreateFileStream("task1-1-n1.tr"),star.GetSpokeNode(0)->GetDevice(0));       
        pointToPoint.EnableAscii(ascii.CreateFileStream("task1-1-n2.tr"),star.GetSpokeNode(1)->GetDevice(0));     
        csma2.EnableAscii(ascii.CreateFileStream("task1-1-n8.tr"),csmaDevices2.Get(1));
        csma2.EnableAscii(ascii.CreateFileStream("task1-1-n9.tr"),csmaDevices2.Get(2));
    }
    
/// Inizio configurazione 2 :  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if ( configuration == 2 ) {

    //  UDP Echo Server su n2, porta 63   
        UdpEchoServerHelper echoServer(63);

        ApplicationContainer serverApps = echoServer.Install(star.GetSpokeNode(1));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(20.0));

    //  UDP Echo Client su n8
        uint32_t maxPacketCount = 5;
        Time interPacketInterval = Seconds(2.);
        UdpEchoClientHelper echoClient(star.GetSpokeIpv4Address(1), 63);
        echoClient.SetAttribute("PacketSize", UintegerValue(2560));
        echoClient.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        echoClient.SetAttribute("Interval", TimeValue(interPacketInterval));

        ApplicationContainer clientApps = echoClient.Install(csmaNodes2.Get(1));
        clientApps.Start(Seconds(3.0));
        clientApps.Stop(Seconds(15.0));

    //  TCP Sink su n1, porta 2600
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

    //  Impostazione dell' OnOff sul nodo n9
        ApplicationContainer clientApps3 = clientHelper3.Install(csmaNodes2.Get(2));
        AddressValue remoteAddress3(InetSocketAddress(csmaInterfaces2.GetAddress(2) , port3));
        clientHelper3.SetAttribute("Remote", remoteAddress3);
        clientApps3.Start(Seconds(3.0));
        clientApps3.Stop(Seconds(9.0));

    //  UDP Sink su n3, porta 2500
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

    //  Impostazione dell' OnOff sul nodo n8
        ApplicationContainer clientApps4 = clientHelper4.Install(csmaNodes2.Get(1));
        AddressValue remoteAddress4(InetSocketAddress(csmaInterfaces2.GetAddress(1) , port4));
        clientHelper4.SetAttribute("Remote", remoteAddress4);
        clientApps4.Start(Seconds(5.0));
        clientApps4.Stop(Seconds(15.0));
    
    //  Setting the message in the UDP packets
        std::string matricola = " Somma delle matricole : 5823635 ";
        int size = 2559 - matricola.size();
        for (int i = 0; i < size; i++){
            matricola += char(0);
        }
        echoClient.SetFill(clientApps.Get(0), matricola);

        NS_LOG_INFO("Enable static global routing.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    //  Cattura i pacchetti e crea un file .pcap
        NS_LOG_INFO("Enable pcap tracing.");

    //  TEST PCAP sui nodi n0, n5, n7
        pointToPoint.EnablePcap("task1-2-n0.pcap", star.GetHub()->GetDevice(0), true, true);
        csma1.EnablePcap("task1-2-n5.pcap",csmaDevices1.Get(1),true,true);
        pointToPoint.EnablePcap("task1-2-n7.pcap",p2pDevices.Get(1),true,true);

    //  ASCII Tracing su server n1 n2 n3 e client n8 n9
        AsciiTraceHelper ascii;
        pointToPoint.EnableAscii(ascii.CreateFileStream("task1-2-n1.tr"),star.GetSpokeNode(0)->GetDevice(0));       
        pointToPoint.EnableAscii(ascii.CreateFileStream("task1-2-n2.tr"),star.GetSpokeNode(1)->GetDevice(0));     
        pointToPoint.EnableAscii(ascii.CreateFileStream("task1-2-n3.tr"),star.GetSpokeNode(2)->GetDevice(0));       
        csma2.EnableAscii(ascii.CreateFileStream("task1-2-n8.tr"),csmaDevices2.Get(1));
        csma2.EnableAscii(ascii.CreateFileStream("task1-2-n9.tr"),csmaDevices2.Get(2));
    }

/// Printando indirizzi IP dei nodi  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

// ip.src==192.118.2.3 && tcp.srcport==49153 && ip.dst==10.0.1.2 && tcp.dstport==2600