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

//  Creando 2 nodi
    NodeContainer nodes;
    nodes.Create(2);

    if ( *argv[1] == '0' ) {
        
//  Creando "network cable" con DataRate e Delay
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

//  Installando la point-to-point tra i nodi
    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

//  Installando una Stack Internet (TCO,UDP,IP, ecc..) su ognuno dei nodi nel NodeContainer
    InternetStackHelper stack;
    stack.Install(nodes);

//  Associando gli indirizzi IP ai devices partendo dal network 10.1.1.0 usando una maschera 255.255.255.0 per definire i bit allocabili
//  Di default gli indirizzi partono da .1 e incrementano monotonamente
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

//  Settando un EDP echo server sul nodo 1 che abbiamo creato che genera traffico dopo 1 sec e termina dopo 10 sec 
    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

//  Settando un EDP echo client con Remote Address del nodo 1 e Remote Port 9?
//  Con max numero di pacchetti, con intervallo di tempo tra un pacchetto e l'altro, e il payload di un pacchetto
//  Settando un EDP echo client sul nodo 0 che abbiamo creato che riceve traffico dopo 2 sec e termina dopo 10 sec

    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

//  Cattura i pacchetti e crea un file .pcap
    pointToPoint.EnablePcapAll("task1");
    
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
