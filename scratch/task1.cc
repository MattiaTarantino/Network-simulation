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




/*
+1000ns 7.55185e+14
+2.00001e+08ns 9.22337e+13
+4.00001e+08ns 13.08
+6.00001e+08ns 9.96
+8.00001e+08ns 8.04
+1e+09ns 8.34
+1.2e+09ns 10.5
+1.4e+09ns 9.84
+1.6e+09ns 10.02
+1.8e+09ns 9.96
+2e+09ns 9.96
+2.2e+09ns 10.38
+2.4e+09ns 9.6
+2.6e+09ns 9.96
+2.8e+09ns 10.02
+3e+09ns 9.96
+3.2e+09ns 10.38
+3.4e+09ns 9.6
+3.6e+09ns 10.02
+3.8e+09ns 9.96
+4e+09ns 10.02
+4.2e+09ns 10.08
+4.4e+09ns 9.84
+4.6e+09ns 10.02
+4.8e+09ns 10.2
+5e+09ns 9.78
+5.2e+09ns 9.96
+5.4e+09ns 10.02
+5.6e+09ns 9.96
+5.8e+09ns 10.44
+6e+09ns 9.54
+6.2e+09ns 9.96
+6.4e+09ns 10.02
+6.6e+09ns 10.02
+6.8e+09ns 10.26
+7e+09ns 9.66
+7.2e+09ns 10.02
+7.4e+09ns 10.02
+7.6e+09ns 9.96
+7.8e+09ns 10.02
+8e+09ns 9.9
+8.2e+09ns 10.02
+8.4e+09ns 10.26
+8.6e+09ns 9.72
+8.8e+09ns 9.96
+9e+09ns 10.02
+9.2e+09ns 9.96
+9.4e+09ns 10.5
+9.6e+09ns 9.48
+9.8e+09ns 10.02
+1e+10ns 9.96
+1.02e+10ns 7.2
+1.04e+10ns 0.96
+1.06e+10ns 10.14
+1.08e+10ns 9.9
+1.1e+10ns 10.02
+1.12e+10ns 9.96
+1.14e+10ns 10.02
+1.16e+10ns 10.5
+1.18e+10ns 9.42
+1.2e+10ns 10.02
+1.22e+10ns 9.96
+1.24e+10ns 10.02
+1.26e+10ns 10.02
+1.28e+10ns 9.96
+1.3e+10ns 10.32
+1.32e+10ns 9.66
+1.34e+10ns 9.96
+1.36e+10ns 9.96
+1.38e+10ns 10.02
+1.4e+10ns 10.2
+1.42e+10ns 9.78
+1.44e+10ns 10.14
+1.46e+10ns 9.84
+1.48e+10ns 9.96
+1.5e+10ns 10.02
+1.52e+10ns 9.96
+1.54e+10ns 10.44
+1.56e+10ns 9.54
+1.58e+10ns 9.96
+1.6e+10ns 10.02
+1.62e+10ns 9.96
+1.64e+10ns 10.02
+1.66e+10ns 9.96
+1.68e+10ns 10.44
+1.7e+10ns 9.54
+1.72e+10ns 9.96
+1.74e+10ns 10.02
+1.76e+10ns 9.96
+1.78e+10ns 10.14
+1.8e+10ns 9.84
+1.82e+10ns 10.26
+1.84e+10ns 9.72
+1.86e+10ns 10.02
+1.88e+10ns 9.96
+1.9e+10ns 9.96
+1.92e+10ns 10.38
+1.94e+10ns 9.6
+1.96e+10ns 10.08
+1.98e+10ns 9.9
+2e+10ns 10.02
+2.02e+10ns 9.96
+2.04e+10ns 10.02
+2.06e+10ns 6.84
+2.08e+10ns 1.56
+2.1e+10ns 10.44
+2.12e+10ns 9.48
+2.14e+10ns 10.08
+2.16e+10ns 9.96
+2.18e+10ns 9.96
+2.2e+10ns 10.02
+2.22e+10ns 9.96
+2.24e+10ns 10.44
+2.26e+10ns 9.54
+2.28e+10ns 10.02
+2.3e+10ns 9.96
+2.32e+10ns 10.02
+2.34e+10ns 10.14
+2.36e+10ns 9.78
+2.38e+10ns 10.26
+2.4e+10ns 9.72
+2.42e+10ns 10.02
+2.44e+10ns 9.96
+2.46e+10ns 10.02
+2.48e+10ns 10.32
+2.5e+10ns 9.66
+2.52e+10ns 10.02
+2.54e+10ns 9.9
+2.56e+10ns 10.02
+2.58e+10ns 9.96
+2.6e+10ns 10.02
+2.62e+10ns 10.5
+2.64e+10ns 9.48
+2.66e+10ns 9.96
+2.68e+10ns 10.02
+2.7e+10ns 9.96
+2.72e+10ns 10.08
+2.74e+10ns 9.9
+2.76e+10ns 10.32
+2.78e+10ns 9.66
+2.8e+10ns 9.96
+2.82e+10ns 10.02
+2.84e+10ns 9.96
+2.86e+10ns 10.26
+2.88e+10ns 9.72
+2.9e+10ns 10.14
+2.92e+10ns 9.84
+2.94e+10ns 9.96
+2.96e+10ns 10.02
+2.98e+10ns 9.96
+3e+10ns 10.5
+3.02e+10ns 9.48
+3.04e+10ns 10.02
+3.06e+10ns 9.96
+3.08e+10ns 9.96
+3.1e+10ns 8.22
+3.12e+10ns 1.14
+3.14e+10ns 8.64
+3.16e+10ns 10.14
+3.18e+10ns 9.96
+3.2e+10ns 10.08
+3.22e+10ns 10.32
+3.24e+10ns 9.66
+3.26e+10ns 10.08
+3.28e+10ns 9.9
+3.3e+10ns 10.02
+3.32e+10ns 9.96
+3.34e+10ns 9.96
+3.36e+10ns 10.5
+3.38e+10ns 9.48
+3.4e+10ns 10.02
+3.42e+10ns 9.96
+3.44e+10ns 10.02
+3.46e+10ns 10.02
+3.48e+10ns 9.96
+3.5e+10ns 10.32
+3.52e+10ns 9.6
+3.54e+10ns 10.02
+3.56e+10ns 9.96
+3.58e+10ns 10.02
+3.6e+10ns 10.2
+3.62e+10ns 9.78
+3.64e+10ns 10.14
+3.66e+10ns 9.84
+3.68e+10ns 9.96
+3.7e+10ns 9.96
+3.72e+10ns 10.02
+3.74e+10ns 10.38
+3.76e+10ns 9.6
+3.78e+10ns 9.96
+3.8e+10ns 10.02
+3.82e+10ns 9.96
+3.84e+10ns 10.02
+3.86e+10ns 9.96
+3.88e+10ns 10.44
+3.9e+10ns 9.54
+3.92e+10ns 9.96
+3.94e+10ns 10.02
+3.96e+10ns 9.96
+3.98e+10ns 10.14
+4e+10ns 9.84
+4.02e+10ns 10.26
+4.04e+10ns 9.72
+4.06e+10ns 9.96
+4.08e+10ns 10.02
+4.1e+10ns 9.96
+4.12e+10ns 10.32
+4.14e+10ns 9.66
+4.16e+10ns 4.68
+4.18e+10ns 3.72
+4.2e+10ns 10.14
+4.22e+10ns 10.02
+4.24e+10ns 10.08
+4.26e+10ns 10.44
+4.28e+10ns 9.54
+4.3e+10ns 9.96
+4.32e+10ns 9.96
+4.34e+10ns 10.02
+4.36e+10ns 9.96
+4.38e+10ns 10.02
+4.4e+10ns 10.38
+4.42e+10ns 9.6
+4.44e+10ns 9.96
+4.46e+10ns 10.02
+4.48e+10ns 9.96
+4.5e+10ns 10.2
+4.52e+10ns 9.78
+4.54e+10ns 10.2
+4.56e+10ns 9.78
+4.58e+10ns 9.96
+4.6e+10ns 10.02
+4.62e+10ns 9.96
+4.64e+10ns 10.38
+4.66e+10ns 9.6
+4.68e+10ns 9.96
+4.7e+10ns 10.02
+4.72e+10ns 9.96
+4.74e+10ns 10.02
+4.76e+10ns 9.96
+4.78e+10ns 10.5
+4.8e+10ns 9.48
+4.82e+10ns 10.02
+4.84e+10ns 9.96
+4.86e+10ns 9.96
+4.88e+10ns 10.14
+4.9e+10ns 9.84
+4.92e+10ns 10.26
+4.94e+10ns 9.72
+4.96e+10ns 10.02
+4.98e+10ns 9.96
+5e+10ns 10.02
+5.02e+10ns 10.26
+5.04e+10ns 9.66
+5.06e+10ns 10.08
+5.08e+10ns 9.9
+5.1e+10ns 10.02
+5.12e+10ns 9.96
+5.14e+10ns 10.02
+5.16e+10ns 10.5
+5.18e+10ns 9.48
+5.2e+10ns 4.08
+5.22e+10ns 4.26
+5.24e+10ns 10.08
+5.26e+10ns 10.14
+5.28e+10ns 10.02
+5.3e+10ns 10.44
+5.32e+10ns 9.54
+5.34e+10ns 9.96
+5.36e+10ns 10.02
+5.38e+10ns 9.96
+5.4e+10ns 10.14
+5.42e+10ns 9.84
+5.44e+10ns 10.2
+5.46e+10ns 9.78
+5.48e+10ns 9.96
+5.5e+10ns 10.02
+5.52e+10ns 9.96
+5.54e+10ns 10.38
+5.56e+10ns 9.6
+5.58e+10ns 10.02
+5.6e+10ns 9.96
+5.62e+10ns 9.96
+5.64e+10ns 10.02
+5.66e+10ns 9.96
+5.68e+10ns 10.5
+5.7e+10ns 9.48
+5.72e+10ns 10.02
+5.74e+10ns 9.96
+5.76e+10ns 10.02
+5.78e+10ns 10.02
+5.8e+10ns 9.9
+5.82e+10ns 10.32
+5.84e+10ns 9.66
+5.86e+10ns 10.02
+5.88e+10ns 9.96
+5.9e+10ns 10.02
+5.92e+10ns 10.26
+5.94e+10ns 9.72
+5.96e+10ns 10.08
+5.98e+10ns 9.84
+6e+10ns 10.02
+6.02e+10ns 9.96
+6.04e+10ns 10.02
+6.06e+10ns 10.44
+6.08e+10ns 9.54
+6.1e+10ns 9.96
+6.12e+10ns 10.02
+6.14e+10ns 9.96
+6.16e+10ns 9.96
+6.18e+10ns 10.02
+6.2e+10ns 10.38
+6.22e+10ns 9.6
+6.24e+10ns 3.78
+6.26e+10ns 4.56
+6.28e+10ns 10.26
+6.3e+10ns 9.78
+6.32e+10ns 10.02
+6.34e+10ns 10.02
+6.36e+10ns 9.96
+6.38e+10ns 10.32
+6.4e+10ns 9.66
+6.42e+10ns 10.02
+6.44e+10ns 9.96
+6.46e+10ns 9.96
+6.48e+10ns 10.02
+6.5e+10ns 9.96
+6.52e+10ns 10.56
+6.54e+10ns 9.42
+6.56e+10ns 9.96
+6.58e+10ns 10.02
+6.6e+10ns 9.96
+6.62e+10ns 10.08
+6.64e+10ns 9.9
+6.66e+10ns 10.32
+6.68e+10ns 9.66
+6.7e+10ns 10.02
+6.72e+10ns 9.96
+6.74e+10ns 10.02
+6.76e+10ns 10.2
+6.78e+10ns 9.72
+6.8e+10ns 10.14
+6.82e+10ns 9.84
+6.84e+10ns 10.02
+6.86e+10ns 9.96
+6.88e+10ns 10.02
+6.9e+10ns 10.44
+6.92e+10ns 9.54
+6.94e+10ns 9.96
+6.96e+10ns 9.96
+6.98e+10ns 10.02
+7e+10ns 9.96
+7.02e+10ns 10.02
+7.04e+10ns 10.38
+7.06e+10ns 9.6
+7.08e+10ns 9.96
+7.1e+10ns 10.02
+7.12e+10ns 9.96
+7.14e+10ns 10.14
+7.16e+10ns 9.84
+7.18e+10ns 10.2
+7.2e+10ns 9.78
+7.22e+10ns 9.96
+7.24e+10ns 10.02
+7.26e+10ns 9.96
+7.28e+10ns 7.08
+7.3e+10ns 1.38
+7.32e+10ns 9.84
+7.34e+10ns 9.96
+7.36e+10ns 10.56
+7.38e+10ns 9.48
+7.4e+10ns 9.96
+7.42e+10ns 10.02
+7.44e+10ns 9.96
+7.46e+10ns 10.02
+7.48e+10ns 9.96
+7.5e+10ns 10.38
+7.52e+10ns 9.6
+7.54e+10ns 9.96
+7.56e+10ns 10.02
+7.58e+10ns 9.96
+7.6e+10ns 10.26
+7.62e+10ns 9.72
+7.64e+10ns 10.14
+7.66e+10ns 9.84
+7.68e+10ns 10.02
+7.7e+10ns 9.96
+7.72e+10ns 9.96
+7.74e+10ns 10.44
+7.76e+10ns 9.54
+7.78e+10ns 10.02
+7.8e+10ns 9.96
+7.82e+10ns 10.02
+7.84e+10ns 9.96
+7.86e+10ns 10.02
+7.88e+10ns 10.38
+7.9e+10ns 9.54
+7.92e+10ns 10.02
+7.94e+10ns 9.96
+7.96e+10ns 10.02
+7.98e+10ns 10.14
+8e+10ns 9.84
+8.02e+10ns 10.2
+8.04e+10ns 9.78
+8.06e+10ns 9.96
+8.08e+10ns 9.96
+8.1e+10ns 10.02
+8.12e+10ns 10.32
+8.14e+10ns 9.66
+8.16e+10ns 10.02
+8.18e+10ns 9.96
+8.2e+10ns 9.96
+8.22e+10ns 10.02
+8.24e+10ns 9.96
+8.26e+10ns 10.5
+8.28e+10ns 9.48
+8.3e+10ns 9.96
+8.32e+10ns 10.02
+8.34e+10ns 1.44
+8.36e+10ns 6.66
+8.38e+10ns 10.14
+8.4e+10ns 9.96
+8.42e+10ns 10.08
+8.44e+10ns 10.26
+8.46e+10ns 9.72
+8.48e+10ns 10.08
+8.5e+10ns 9.9
+8.52e+10ns 9.96
+8.54e+10ns 10.02
+8.56e+10ns 9.96
+8.58e+10ns 10.5
+8.6e+10ns 9.48
+8.62e+10ns 9.96
+8.64e+10ns 10.02
+8.66e+10ns 9.96
+8.68e+10ns 10.02
+8.7e+10ns 9.96
+8.72e+10ns 10.38
+8.74e+10ns 9.6
+8.76e+10ns 10.02
+8.78e+10ns 9.96
+8.8e+10ns 9.96
+8.82e+10ns 10.2
+8.84e+10ns 9.78
+8.86e+10ns 10.2
+8.88e+10ns 9.78
+8.9e+10ns 10.02
+8.92e+10ns 9.96
+8.94e+10ns 10.02
+8.96e+10ns 10.38
+8.98e+10ns 9.54
+9e+10ns 10.02
+9.02e+10ns 9.96
+9.04e+10ns 10.02
+9.06e+10ns 9.96
+9.08e+10ns 10.02
+9.1e+10ns 10.44
+9.12e+10ns 9.54
+9.14e+10ns 9.96
+9.16e+10ns 9.96
+9.18e+10ns 10.02
+9.2e+10ns 10.08
+9.22e+10ns 9.9
+9.24e+10ns 10.26
+9.26e+10ns 9.72
+9.28e+10ns 9.96
+9.3e+10ns 10.02
+9.32e+10ns 9.96
+9.34e+10ns 10.32
+9.36e+10ns 9.66
+9.38e+10ns 3.78
+9.4e+10ns 4.56
+9.42e+10ns 10.14
+9.44e+10ns 9.96
+9.46e+10ns 10.08
+9.48e+10ns 10.2
+9.5e+10ns 9.78
+9.52e+10ns 10.14
+9.54e+10ns 9.78
+9.56e+10ns 10.02
+9.58e+10ns 9.96
+9.6e+10ns 10.02
+9.62e+10ns 10.38
+9.64e+10ns 9.6
+9.66e+10ns 9.96
+9.68e+10ns 10.02
+9.7e+10ns 9.96
+9.72e+10ns 9.96
+9.74e+10ns 10.02
+9.76e+10ns 10.44
+9.78e+10ns 9.54
+9.8e+10ns 9.96
+9.82e+10ns 10.02
+9.84e+10ns 9.96
+9.86e+10ns 10.14
+9.88e+10ns 9.84
+9.9e+10ns 10.26
+9.92e+10ns 9.72
+9.94e+10ns 9.96
+9.96e+10ns 10.02
+9.98e+10ns 9.96
*/