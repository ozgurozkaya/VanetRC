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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("VanetRC");

class RoutingExample{
  public:
    void run();

  private:
    // parameters
    /// Number of nodes
    uint32_t size = 25;
    /// Distance between nodes, meters
    //double step = 25;
    /// Simulation time, seconds
    double totalTime = 10;
    /// Write per-device PCAP traces if true
    bool pcap = true;
    /// Print routes if true
    bool printRoutes = true;
    ///Size of Packet (bytes), Packet interval (Time), Max Packets
    double packet_size = 512;
    Time packet_interval = MilliSeconds (500);
    double max_packets = 50;
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
  installApplications();
  if(pcap) enablePcapTracing();
  if(printRoutes) printingRoutingTable();
  
  AnimationInterface anim (animFile);
  anim.EnablePacketMetadata (); // Optional
  anim.EnableIpv4RouteTracking ("xml/vanetRC-routingtable.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
  //anim.SetStartTime (Seconds(0.0));
  //anim.SetStopTime (Seconds(10.0));

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  std::cout << "Packets Received: \t" << sink->GetTotalRx() / double(packet_size) << "\n";
  std::cout << "Throughput: \t\t" << sink->GetTotalRx() / (totalTime - 2) << " Bytes/sec \n";
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));

  //WifiHelper wifi;
  //wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
  //  StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (2500));
  devices = wifi.Install (wifiPhy, wifiMac, nodes); 
};

void
RoutingExample::installInternetStack(){
  stack.SetRoutingHelper (routing); // has effect on the next Install ()
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);
};

void 
RoutingExample::installApplications(){
  
  UdpEchoServerHelper echoServer (9);
  
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (max_packets));
  echoClient.SetAttribute ("Interval", TimeValue (packet_interval));
  echoClient.SetAttribute ("PacketSize", UintegerValue (packet_size));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (size-1));

  PacketSinkHelper sinkHelper("ns3::UdpSocketFactory",  InetSocketAddress (interfaces.GetAddress (0), 9));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(0));


  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (totalTime));

  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (totalTime));

  sink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (1.0));
  sinkApp.Stop(Seconds(totalTime));

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