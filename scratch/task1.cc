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
    cmd.Parse (argc, argv);                    

    Time::SetResolution(Time::NS);                                                 
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

//  Creo i nodi che compongono la stella, escluso il nodo centrale n0
    uint32_t nSpokes = 4;

//  Creo i nodi n6 e n7 che andranno a instaurare una connessione point-to-point
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

//  Creo i nodi che compongono la prima LAN, escluso n4 essendo già considerato nella stella e n6 dato che verrà creato con una connessione point to point
    uint32_t nCsma1 = 1;

//  ??????????
    nCsma1 = nCsma1 == 0 ? 1 : nCsma1;

//  Creo i nodi che compongono la seconda LAN, escluso n7 essendo già considerato nella connessione point-to-point
    uint32_t nCsma2 = 2;

//  ??????????
    nCsma2 = nCsma2 == 0 ? 1 : nCsma2;

//  Creo la stella n0-n{1,2,3,4} e configuro i parametri
    NS_LOG_INFO("Build star topology.");
    PointToPointHelper pointToPoint1;
    pointToPoint1.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
    pointToPoint1.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));
    PointToPointStarHelper star(nSpokes, pointToPoint1);

//  Creo la prima LAN partendo da n4 e aggiungendo anche n6 facente parte della connessione point-to-point 
    NodeContainer csmaNodes1;
    csmaNodes1.Add(star.GetSpokeNode(3));
    csmaNodes1.Create(nCsma1);
    csmaNodes1.Add(p2pNodes.Get(0));

//  Configuro i parametri della prima LAN
    CsmaHelper csma1;
    csma1.SetChannelAttribute("DataRate", StringValue("25Mbps"));
    csma1.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));

//  Installo la Csma sui nodi csmaNodes1
    NetDeviceContainer csmaDevices1;
    csmaDevices1 = csma1.Install(csmaNodes1);

//  Creo la seconda LAN partendo da n7
    NodeContainer csmaNodes2;
    csmaNodes2.Add(p2pNodes.Get(1));
    csmaNodes2.Create(nCsma2);

//  Configuro i parametri della connessione point-to-point tra n6 e n7
    PointToPointHelper pointToPoint0;
    pointToPoint0.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
    pointToPoint0.SetChannelAttribute("Delay", TimeValue(MicroSeconds(10)));

//  Configuro i parametri della seconda LAN
    CsmaHelper csma2;
    csma2.SetChannelAttribute("DataRate", StringValue("30Mbps"));
    csma2.SetChannelAttribute("Delay", TimeValue(MicroSeconds(20)));

//  Installo la Csma sui nodi csmaNodes2
    NetDeviceContainer csmaDevices2;
    csmaDevices2 = csma2.Install(csmaNodes2);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//  Installando una Stack Internet (TCO,UDP,IP, ecc..) su ognuno dei nodi nel NodeContainer
    NS_LOG_INFO("Install internet stack on all nodes.");
    InternetStackHelper stack;
    star.InstallStack(stack);
    stack.Install(csmaNodes1);
    stack.Install(p2pNodes);
    stack.Install(csmaNodes2);

//  Associando gli indirizzi IP ai devices partendo dal network 10.0.1.0 usando una maschera 255.255.255.0 per definire i bit allocabili
//  Di default gli indirizzi partono da .1 e incrementano monotonamente
    NS_LOG_INFO("Assign IP Addresses.");
    star.AssignIpv4Addresses(Ipv4AddressHelper("10.0.1.0", "255.255.255.0"));

    Ipv4AddressHelper address;
    address.SetBase("192.118.1.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces1 = address.Assign(csmaDevices1);

    address.SetBase("10.0.2.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("192.118.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces2 = address.Assign(csmaDevices2);

//  Inizio configurazione 0 :
    if ( *argv[1] == '0' ) {

//  Settando un UDP echo server sul nodo 1 che abbiamo creato che genera traffico dopo 1 sec e termina dopo 10 sec 
    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

//  Settando un UDP echo client con Remote Address del nodo 1 e Remote Port 9?
//  Con max numero di pacchetti, con intervallo di tempo tra un pacchetto e l'altro, e il payload di un pacchetto
//  Settando un UDP echo client sul nodo 0 che abbiamo creato che riceve traffico dopo 2 sec e termina dopo 10 sec

    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

//  Cattura i pacchetti e crea un file .pcap
    pointToPoint.EnablePcapAll("task");

//  ASCII Tracing    
    AsciiTraceHelper ascii;
    pointToPoint.EnableAsciiAll(ascii.CreateFileStream("task.tr"));

    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
