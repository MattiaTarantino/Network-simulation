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

NS_LOG_COMPONENT_DEFINE("TaskScript");

int main(int argc, char* argv[]){

//  Configurazione impostabile dinamicamente da linea di comando
    CommandLine cmd;
    int configuration = 2;
    cmd.AddValue("configuration", "numero configurazione", configuration);
    cmd.Parse (argc, argv);                    

    Time::SetResolution(Time::NS);                                                 
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

//  Creando i nodi che compongono la stella, escluso il nodo centrale n0
    uint32_t nSpokes = 4;

//  Creando i nodi che compongono la prima LAN, escluso n4 essendo già considerato nella stella e n6 dato che verrà creato con una connessione point to point
    uint32_t nCsma1 = 1;

//  ??????????
    nCsma1 = nCsma1 == 0 ? 1 : nCsma1;

//  Creando i nodi che compongono la seconda LAN, escluso n7 essendo già considerato nella connessione point-to-point
    uint32_t nCsma2 = 2;

//  ??????????
    nCsma2 = nCsma2 == 0 ? 1 : nCsma2;

//  Creando la stella n0-n{1,2,3,4} e configuro i parametri
    NS_LOG_INFO("Build star topology.");
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10us"));
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
/*
//  Configurando i parametri della connessione point-to-point tra n6 e n7
    PointToPointHelper pointToPoint_6_7;
    pointToPoint_6_7.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
    pointToPoint_6_7.SetChannelAttribute("Delay", StringValue("10us"));     

//  Installando un point-to-point net device sui nodi n6 e n7 e un canale point-to-point tra essi
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint_6_7.Install(p2pNodes);          */
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

        // Creazione di un packet sink sul nodo n1 della stella, per ricevere i pacchetti
        uint16_t port = 2600;
        Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
        ApplicationContainer sinkApp = sinkHelper.Install(star.GetSpokeNode(0));
        sinkApp.Start(Seconds(1.0));
        sinkApp.Stop(Seconds(20.0));

        // Creazione di un' applicazioni OnOff per mandare TCP al nodo n1
        OnOffHelper clientHelper("ns3::TcpSocketFactory", Address());
        clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize = 1500;
        clientHelper.SetAttribute("PacketSize", UintegerValue(packetSize));

        // Impostazione dell' OnOff sul nodo n9
        ApplicationContainer clientApps;
        AddressValue remoteAddress(InetSocketAddress(csmaInterfaces2.GetAddress(2) , port));
        clientHelper.SetAttribute("Remote", remoteAddress);
        clientApps.Add(clientHelper.Install(csmaNodes2.Get(2)));
        clientApps.Start(Seconds(3.0));
        clientApps.Stop(Seconds(15.0));

        /* PROVA UDP

        UdpEchoServerHelper echoServer(9);

        ApplicationContainer serverApps = echoServer.Install(star.GetSpokeNode(0));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(20.0));

        UdpEchoClientHelper echoClient(star.GetSpokeIpv4Address(0), 9);
        echoClient.SetAttribute("PacketSize", UintegerValue(1500));

        ApplicationContainer clientApps = echoClient.Install(csmaNodes2.Get(2));
        clientApps.Start(Seconds(3.0));
        clientApps.Stop(Seconds(15.0));*/

        NS_LOG_INFO("Enable static global routing.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

        //  Cattura i pacchetti e crea un file .pcap
        NS_LOG_INFO("Enable pcap tracing.");
        //pointToPoint1.EnablePcap("task1-0-0.pcap",star.GetHub(),true);

        // TEST PCAP client server
        /*csma1.EnablePcap("task1-0-5",csmaDevices1.Get(2),true);
        pointToPoint0.EnablePcap("task1-0-7",p2pDevices.Get(1),true);*/

   //     pointToPoint_6_7.EnablePcapAll("pointToPoint_6_7");
        csma1.EnablePcapAll("csma1");
        pointToPoint.EnablePcapAll("pointToPoint_star");
        csma2.EnablePcapAll("csma2");

        /*  ASCII Tracing    
        AsciiTraceHelper ascii;
        //pointToPoint1.EnableAscii(ascii.CreateFileStream("task1-0-1.tr"),star.GetSpokeNode(0));
        //csma2.EnableAscii(ascii.CreateFileStream("task1-0-9.tr"),csmaDevices2.Get(3));

        pointToPoint_6_7.EnableAsciiAll("pointToPoint_6_7");
        csma1.EnableAsciiAll("csma1");
        pointToPoint_star.EnableAsciiAll("pointToPoint_star");
        csma2.EnableAsciiAll("csma2");*/
    }

//  Inizio configurazione 1 :   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if ( configuration == 1 ) {

        // Creazione di un packet sink sul nodo n1 della stella, per ricevere i pacchetti
        uint16_t port1 = 2600;
        Address sinkLocalAddress1(InetSocketAddress(Ipv4Address::GetAny(), port1));
        PacketSinkHelper sinkHelper1("ns3::TcpSocketFactory", sinkLocalAddress1);
        ApplicationContainer sinkApp1 = sinkHelper1.Install(star.GetSpokeNode(0));
        sinkApp1.Start(Seconds(1.0));
        sinkApp1.Stop(Seconds(20.0));

        // Creazione di un' applicazioni OnOff per mandare TCP al nodo n1
        OnOffHelper clientHelper1("ns3::TcpSocketFactory", Address());
        clientHelper1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize1 = 2500;
        clientHelper1.SetAttribute("PacketSize", UintegerValue(packetSize1));

        // Impostazione dell' OnOff sul nodo n9
        ApplicationContainer clientApps1;
        AddressValue remoteAddress1(InetSocketAddress(csmaInterfaces2.GetAddress(2) , port1));
        clientHelper1.SetAttribute("Remote", remoteAddress1);
        clientApps1.Add(clientHelper1.Install(csmaNodes2.Get(2)));
        clientApps1.Start(Seconds(5.0));
        clientApps1.Stop(Seconds(15.0));

        // Creazione di un packet sink sul nodo n2 della stella, per ricevere i pacchetti
        uint16_t port2 = 7777;
        Address sinkLocalAddress2(InetSocketAddress(Ipv4Address::GetAny(), port2));
        PacketSinkHelper sinkHelper2("ns3::TcpSocketFactory", sinkLocalAddress2);
        ApplicationContainer sinkApp2 = sinkHelper2.Install(star.GetSpokeNode(1));
        sinkApp2.Start(Seconds(1.0));
        sinkApp2.Stop(Seconds(20.0));

        // Creazione di un' applicazioni OnOff per mandare TCP al nodo n2
        OnOffHelper clientHelper2("ns3::TcpSocketFactory", Address());
        clientHelper2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        clientHelper2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        uint32_t packetSize2 = 5000;
        clientHelper2.SetAttribute("PacketSize", UintegerValue(packetSize2));

        // Impostazione dell' OnOff sul nodo n8
        ApplicationContainer clientApps2;
        AddressValue remoteAddress2(InetSocketAddress(csmaInterfaces2.GetAddress(1) , port2));
        clientHelper2.SetAttribute("Remote", remoteAddress2);
        clientApps2.Add(clientHelper2.Install(csmaNodes2.Get(1)));
        clientApps2.Start(Seconds(2.0));
        clientApps2.Stop(Seconds(9.0));

        NS_LOG_INFO("Enable static global routing.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

        //  Cattura i pacchetti e crea un file .pcap
        NS_LOG_INFO("Enable pcap tracing.");
        //pointToPoint1.EnablePcap("task1-0-0.pcap",star.GetHub(),true);

        // TEST PCAP client server
        /*csma1.EnablePcap("task1-0-5",csmaDevices1.Get(2),true);
        pointToPoint0.EnablePcap("task1-0-7",p2pDevices.Get(1),true);*/

      //  pointToPoint_6_7.EnablePcapAll("pointToPoint_6_7");
        csma1.EnablePcapAll("csma1");
        pointToPoint.EnablePcapAll("pointToPoint_star");
        csma2.EnablePcapAll("csma2");

        /*  ASCII Tracing    
        AsciiTraceHelper ascii;
        pointToPoint1.EnableAscii(ascii.CreateFileStream("task1-0-1.tr"),star.GetSpokeNode(0));
        csma2.EnableAscii(ascii.CreateFileStream("task1-0-9.tr"),csmaDevices2.Get(3));

        pointToPoint0.EnableAsciiAll("p2p0.tr");
        csma1.EnableAsciiAll("csma1.tr");
        pointToPoint1.EnableAsciiAll("p2p1.tr");
        csma2.EnableAsciiAll("csma2.tr");*/

    }
    
//  Inizio configurazione 2 :   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if ( configuration == 2 ) {

        UdpEchoServerHelper echoServer(63);

        ApplicationContainer serverApps = echoServer.Install(star.GetSpokeNode(1));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(20.0));

        uint32_t maxPacketCount = 5;
        Time interPacketInterval = Seconds(2.);
        UdpEchoClientHelper echoClient(star.GetSpokeIpv4Address(1), 63);
        echoClient.SetAttribute("PacketSize", UintegerValue(2560));
        echoClient.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        echoClient.SetAttribute("Interval", TimeValue(interPacketInterval));

        ApplicationContainer clientApps = echoClient.Install(csmaNodes2.Get(1));
        clientApps.Start(Seconds(3.0));
        clientApps.Stop(Seconds(15.0));

        //echoClient.SetFill(clientApps.Get(0),"5823635");

        pointToPoint.EnablePcapAll("pointToPoint_star");
        csma1.EnablePcap("csma1", csmaDevices1.Get(1), true);
    //    pointToPoint_6_7.EnablePcapAll("pointToPoint_6_7");
        csma2.EnablePcap("csma2", csmaDevices2.Get(1), true);
        csma2.EnablePcap("csma2", csmaDevices2.Get(2), true);
      //  csma1.EnablePcapAll("csma1");
     //   csma2.EnablePcapAll("csma2");

        NS_LOG_INFO("Enable static global routing.");
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    }

//  Printando indirizzi IP dei nodi

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
