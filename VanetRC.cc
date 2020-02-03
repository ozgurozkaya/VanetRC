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
 */

#include <iostream>
#include <string>
#include <cmath>
#include "ns3/aodv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/ssid.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/onoff-application.h"
#include "ns3/random-variable-stream.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("VanetRC");

class RoutingExample{
  public:
    void run();

  private:
    // parameters
    /// Number of nodes
    uint32_t size = 25;
    uint32_t server_node = 0;
    uint32_t client_node = size-1;
    /// Distance between nodes, meters
    //double step = 25;
    /// Simulation time, seconds
    double totalTime = 180;
    double duration;
    /// Write per-device PCAP traces if true & net-anim file generate
    bool pcap = true;
    bool anim = false;
    /// Print routes if true
    bool printRoutes = true;
    ///Size of Packet (bytes), Packet interval (Time), Max Packets
    double packet_size = 2048;
    Time packet_interval = MilliSeconds (1000);
    double max_packets = 250;
    StringValue data_rate = StringValue("448kb/s");
    //Internet Stack Helper
    InternetStackHelper stack;
    //Pointer to the packet sink application 
    Ptr<PacketSink> sink;
    //The value of the last total received bytes
    uint64_t lastTotalRx = 0;

    //Routing Method
    AodvHelper routing;
    //OlsrHelper routing;
    //DsdvHelper routing;
    // you can configure AODV attributes here using aodv.Set(name, value)

  // network
  /// nodes used in the example
  NodeContainer nodes;
  /// devices used in the example
  NetDeviceContainer devices;
  /// interfaces used in the example
  Ipv4InterfaceContainer interfaces;
  
  private:
    /// Create the nodes (i -> quantity)
    void createNodes (int i);
    /// Create the devices
    void createDevices ();
    /// Create the network
    void installInternetStack ();
    /// Create the simulation applications
    void installApplications ();
    void installOnOffApplications ();
    // Saves all nodes' routing tables in a txt file
    void printingRoutingTable ();
    // Saves all nodes' pcap tracing file
    void enablePcapTracing ();
    // Calculate the throughput of network
    void calculateThroughput();
};

int
main (int argc, char *argv[])
{
  // Enable logging for UdpClient and
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
 
  CommandLine cmd;
  cmd.Parse (argc, argv);

  RoutingExample app_RE;
  app_RE.run();
}

void
RoutingExample::run(){
  
  std::string animFile = "xml/test.xml";

  createNodes(size);
  createDevices();
  installInternetStack();
  installOnOffApplications();
  if(pcap) enablePcapTracing();
  if(printRoutes) printingRoutingTable();

  if(anim){ 
    AnimationInterface anim (animFile);
    anim.EnablePacketMetadata (); // Optional
    anim.EnableIpv4RouteTracking ("xml/vanetRC-routingtable.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
    anim.UpdateNodeDescription (nodes.Get (server_node), "Server"); // Optional
    anim.UpdateNodeColor (nodes.Get (server_node), 0, 255, 0); // Optional
    anim.UpdateNodeDescription (nodes.Get (client_node), "Client"); // Optional
    anim.UpdateNodeColor (nodes.Get (client_node), 0, 0, 255); // Optional
    anim.EnableWifiMacCounters (Seconds (0), Seconds (180)); //Optional
    anim.EnableWifiPhyCounters (Seconds (0), Seconds (180)); //Optional
    anim.SetMaxPktsPerTraceFile(50000);
    //anim.SetStartTime (Seconds(0.0));
    //anim.SetStopTime (Seconds(10.0));
  }

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  std::cout << " Packet Size: \t\t" << packet_size << " Bytes, " << packet_size / 1024 << " KiloBytes \n";
  std::cout << " Data Rate: \t\t" << data_rate.Get() << "\n";
  //std::cout << " Packets Received: \t" << sink->GetTotalRx() / double(packet_size) << "\n";
  std::cout << " Bytes Received: \t" << sink->GetTotalRx() << "\n";
  std::cout << " Throughput: \t\t" << sink->GetTotalRx() / 1024 / 15 << " KiloBytes/sec \n";
  Simulator::Destroy ();
};

void
RoutingExample::createNodes(int i){
  //Creating nodes
  nodes.Create(i);

  //Adding Mobility to the created nodes
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  "MinX", DoubleValue (0.0),
  "MinY", DoubleValue (0.0),
  "DeltaX", DoubleValue (65),
  "DeltaY", DoubleValue (65),
  "GridWidth", UintegerValue (5),
  "LayoutType", StringValue ("RowFirst"));

  //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //"Bounds", RectangleValue (Rectangle (-10000,10000,-10000,10000)));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (nodes);
};

void
RoutingExample::createDevices(){
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  
  //wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //Ekleyince çalışmıyor !!!!
  //wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel","Exponent", StringValue ("2.7"));


  wifiPhy.Set ("TxGain", DoubleValue (1.0) );
  wifiPhy.Set ("RxGain", DoubleValue (1.0) );
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1) );
  wifiPhy.Set ("TxPowerEnd", DoubleValue (18) );
  wifiPhy.Set ("TxPowerStart", DoubleValue (18) );
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (7.0) );
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));

  //WifiHelper wifi;
  //wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", 
  //StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (2500));
  devices = wifi.Install (wifiPhy, wifiMac, nodes); 
};

void
RoutingExample::installInternetStack(){
  
  routing.Set ("AllowedHelloLoss", UintegerValue (20));
  routing.Set ("HelloInterval", TimeValue (Seconds (3)));
  routing.Set ("RreqRetries", UintegerValue (5));
  routing.Set ("ActiveRouteTimeout", TimeValue (Seconds (100)));
  routing.Set ("DestinationOnly", BooleanValue (true));
  stack.SetRoutingHelper (routing); // has effect on the next Install ()
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);
};

/*
void 
RoutingExample::installApplications(){
  
  UdpEchoServerHelper echoServer (9);
  
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (server_node));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (server_node), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (max_packets));
  //echoClient.SetAttribute ("Interval", TimeValue (packet_interval));
  echoClient.SetAttribute ("PacketSize", UintegerValue (packet_size));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (client_node));

  PacketSinkHelper sinkHelper("ns3::UdpSocketFactory",  InetSocketAddress (interfaces.GetAddress (server_node), 9));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(server_node));

  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (totalTime));

  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (totalTime));

  sink = StaticCast<PacketSink> (sinkApp.Get (server_node));
  sinkApp.Start (Seconds (1.0));
  sinkApp.Stop(Seconds(totalTime));

};
*/

void 
RoutingExample::installOnOffApplications(){
  
  int m_nconn = 625;
  double start_time, stop_time, duration;

  ApplicationContainer apps [m_nconn];
  ApplicationContainer sink_apps [m_nconn];

  Ptr<UniformRandomVariable> a = CreateObject<UniformRandomVariable>();
  a->SetAttribute("Min", DoubleValue (50));
  a->SetAttribute("Max", DoubleValue(totalTime-15));

  start_time = a->GetValue();
  Ptr<ExponentialRandomVariable> b = CreateObject<ExponentialRandomVariable>();
  b->SetAttribute("Mean", DoubleValue(30));
  duration = b->GetValue()+1;

  if ( (start_time + duration) > (totalTime - 10)){
    stop_time = totalTime-10;
  }else{
    stop_time = start_time + duration;
  }
  start_time = 10;
  duration = 20;
  stop_time = start_time + duration;
  
  std::cout << "\n Start_time: \t\t" << start_time << "s";
  std::cout << "\n Stop_time: \t\t" << stop_time << "s\n";
  OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress(interfaces.GetAddress (server_node), 9)));
  //onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
  //onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));;
  onoff.SetAttribute ("PacketSize", UintegerValue(packet_size));
  onoff.SetAttribute ("DataRate", data_rate);
  
  apps[0] = onoff.Install (nodes.Get(client_node));
  apps[0].Start (Seconds (start_time));
  apps[0].Stop (Seconds (stop_time));
  // Create a packet sink to receive the packets
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress (server_node), 9));
  sink_apps[0] = sinkHelper.Install (nodes.Get(server_node));
  sink = StaticCast<PacketSink> (sink_apps[0].Get(0));
  sink_apps[0].Start(Seconds (start_time));
  sink_apps[0].Stop(Seconds(stop_time));

/*
  for (int i = 0; i < m_nconn; i++)
  {
    start_time = a->GetValue();
    Ptr<ExponentialRandomVariable> b = CreateObject<ExponentialRandomVariable>();
    b->SetAttribute("Mean", DoubleValue(30));
    duration = b->GetValue()+1;

    if ( (start_time + duration) > (totalTime - 10)){
      stop_time = totalTime-10;
    }else{
      stop_time = start_time + duration;
    }

    OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress(interfaces.GetAddress (0), 9)));
    //onoff.SetAttribute ("OnTime", DoubleValue(5));
    //onoff.SetAttribute ("OffTime", DoubleValue(15));
    apps[i] = onoff.Install (nodes.Get(24));
    apps[i].Start (Seconds (start_time));
    apps[i].Stop (Seconds (stop_time));
    // Create a packet sink to receive the packets
    //PacketSinkHelper sink ("ns3::UdpSocketFactory",InetSocketAddress(interfaces.GetAddress (0), 9));
    //ApplicationContainer sinkApp  = sink.Install (nodes.Get (24));
    //sinkApp.Start (Seconds (1.0));
  }
*/

};

void
RoutingExample::printingRoutingTable(){
  Time rtt = Seconds(20.0);
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> rtw = ascii.CreateFileStream ("xml/routing_table");

  routing.PrintRoutingTableAllAt(rtt,rtw);

};

void
RoutingExample::enablePcapTracing(){
  stack.EnablePcapIpv4All ("xml/pcap/internet"); // gets pcap files of all nodes
}

void
RoutingExample::calculateThroughput(){
  
  Time now = Simulator::Now ();                                         // Return the simulator's virtual time.
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;     // Convert Application RX Packets to MBits.
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  //Simulator::Schedule (MilliSeconds (100), &RoutingExample::calculateThroughput);
  
}