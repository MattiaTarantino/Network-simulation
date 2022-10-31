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
    int configuration = 0;
    cmd.AddValue("configuration", "numero configurazione", configuration);
    cmd.Parse (argc, argv);                    

    Time::SetResolution(Time::NS);                                                 
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

//  Creando i nodi che compongono la stella, escluso il nodo centrale n0
    uint32_t nSpokes = 4;

//  Creando i nodi n6 e n7 che andranno a instaurare una connessione point-to-point
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

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
    PointToPointHelper pointToPoint1;
    pointToPoint1.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
    pointToPoint1.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));
    PointToPointStarHelper star(nSpokes, pointToPoint1);

//  Creando la prima LAN partendo da n4 e aggiungendo anche n6 facente parte della connessione point-to-point 
    NodeContainer csmaNodes1;
    csmaNodes1.Add(star.GetSpokeNode(3));
    csmaNodes1.Create(nCsma1);
    csmaNodes1.Add(p2pNodes.Get(0));

//  Configurando i parametri della prima LAN
    CsmaHelper csma1;
    csma1.SetChannelAttribute("DataRate", StringValue("25Mbps"));
    csma1.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));

//  Installando la Csma sui nodi csmaNodes1
    NetDeviceContainer csmaDevices1;
    csmaDevices1 = csma1.Install(csmaNodes1);

//  Creando la seconda LAN partendo da n7
    NodeContainer csmaNodes2;
    csmaNodes2.Add(p2pNodes.Get(1));
    csmaNodes2.Create(nCsma2);

//  Configurando i parametri della connessione point-to-point tra n6 e n7
    PointToPointHelper pointToPoint0;
    pointToPoint0.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
    pointToPoint0.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));

//  Installando un point-to-point net device sui nodi n6 e n7 e un canale point-to-point tra essi
    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint0.Install(p2pNodes);

//  Configurando i parametri della seconda LAN
    CsmaHelper csma2;
    csma2.SetChannelAttribute("DataRate", StringValue("30Mbps"));
    csma2.SetChannelAttribute("Delay", TimeValue(MicroSeconds(20)));

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

//  Inizio configurazione 0 :   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*    if ( configuration == 0 ) {
        // Creazione di un packet sink sul nodo n1 della stella, per ricevere i pacchetti
        NS_LOG_INFO("Create applications.");
        uint16_t port = 2600;
        Address hubLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", hubLocalAddress);
        ApplicationContainer App_n1 = packetSinkHelper.Install(star.GetSpokeNode(1));
        App_n1.Start(Seconds(1.0));
        App_n1.Stop(Seconds(20.0));

        // Creazione di un' applicazioni OnOff per mandare TCP al nodo n1, uno su ogni nodo della stella
        OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address());
        onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        //  Cattura i pacchetti e crea un file .pcap
        NS_LOG_INFO("Enable pcap tracing.");
        //pointToPoint1.EnablePcap("task1-0-0.pcap",star.GetHub(),true);
        csma1.EnablePcap("task1-0",csmaDevices1.Get(0));
        pointToPoint0.EnablePcap("task1-0",p2pDevices.Get(1));

        //  ASCII Tracing    
        AsciiTraceHelper ascii;
        //pointToPoint1.EnableAscii(ascii.CreateFileStream("task1-0-1.tr"),star.GetSpokeNode(0));
        csma2.EnableAscii(ascii.CreateFileStream("task1-0-9.tr"),csmaDevices2.Get(1));
    }*/

    //  Inizio configurazione 1 :   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( configuration == 1 ) {
    
    }
    
    //  Inizio configurazione 2 :   //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if ( configuration == 2 ) {
    
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
