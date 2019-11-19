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

using namespace ns3;

class RoutingExample{
  public:
    //RoutingExample ();

    void run();

  private:
    // parameters
    /// Number of nodes
    uint32_t size = 20;
    /// Distance between nodes, meters
    double step = 25;
    /// Simulation time, seconds
    double totalTime = 10;
    /// Write per-device PCAP traces if true
    bool pcap = 1;
    /// Print routes if true
    bool printRoutes = 1;

    ///Size of Packet, bytes
    double packet_size = 1024;

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
};

int
main (int argc, char *argv[])
{
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
  printingRoutingTable();

  AnimationInterface anim (animFile);
  //anim.SetStartTime (Seconds(0.0));
  //anim.SetStopTime (Seconds(10.0));

  Simulator::Stop (Seconds (60.0));
  Simulator::Run ();
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
  "DeltaX", DoubleValue (step),
  "DeltaY", DoubleValue (50),
  "GridWidth", UintegerValue (7),
  "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  "Bounds", RectangleValue (Rectangle (-10000,10000,-10000,10000)));

  mobility.Install (nodes);
};

void
RoutingExample::createDevices(){
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes); 
};

void
RoutingExample::installInternetStack(){

  InternetStackHelper stack;
  stack.SetRoutingHelper (routing); // has effect on the next Install ()
  stack.Install (nodes);
  if(pcap){
    stack.EnablePcapIpv4All ("xml/pcap/internet"); // gets pcap files of all nodes
  }
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);
};

void 
RoutingExample::installApplications(){

  V4PingHelper ping (interfaces.GetAddress (16));
  ping.SetAttribute ("Verbose", BooleanValue (true));
  ping.SetAttribute ("Size", UintegerValue(packet_size));
  
  ApplicationContainer p = ping.Install (nodes.Get (0));
  p.Start (Seconds (5));
  p.Stop (Seconds (totalTime));

};

void
RoutingExample::printingRoutingTable(){
  Time rtt = Seconds(20.0);
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> rtw = ascii.CreateFileStream ("xml/routing_table");

  routing.PrintRoutingTableAllAt(rtt,rtw);
};