#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");

int main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 3;

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CMS nodes/devices", nCsma);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc, argv);

  // Set up logging
  if (verbose) {
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
  }
  // Set up number of CSMA devices
  nCsma = nCsma == 0 ? 1 : nCsma;

  // Nodes connected by a PointToPoint link
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  // Nodes on a CSMA bus
  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  // Set up point-to-point

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = p2p.Install (p2pNodes);

  // Set up csma

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  // Set up stack
  InternetStackHelper stack;
  stack.Install (p2pNodes.Get (0));
  stack.Install (csmaNodes);

  // Set up address assignment

  // Assign addresses in the 10.1.1.x block to the p2p devices
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  // Assign addresses in the 10.1.2.x block to the CSMA devices
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  // Set up the server
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // Set up the client
  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Internetwork routing. The entire internetwork is accessible in the simulation;
  // global routing runs through all the nodes and takes care of routing for
  // us without our having to configure them.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* The old tracing 
  // Enable pcap tracing. The first line is the usual; it just gives the prefix.
  p2p.EnablePcapAll ("second");
  // The second line is different. CSMA is multi-p2p network: multiple endpoints
  // on a shared medium. We could generate a trace file for each device, or
  // we can put one device into promiscuous mode, where it sniffs the network
  // for all packets and puts them into a single pcap file. The last parameter
  // flags this csmaDevice as being in promiscuous mode.
  csma.EnablePcap ("second", csmaDevices.Get (1), true);
  */

  p2p.EnablePcap("second", p2pNodes.Get (0)->GetId (), 0);
  csma.EnablePcap("second", csmaNodes.Get (nCsma)->GetId (), 0, false);
  csma.EnablePcap("second", csmaNodes.Get (nCsma-1)->GetId (), 0, false);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
