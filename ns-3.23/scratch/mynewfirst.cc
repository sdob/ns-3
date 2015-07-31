#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  uint32_t nPackets = 1;

  CommandLine cmd;
  cmd.AddValue("nPackets", "Number of packets to echo", nPackets);
  cmd.Parse(argc, argv);

  // We could just use stdout to log if we wanted:
  //std::cout << "The number of packets is " << nPackets << std::endl;

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NS_LOG_INFO ("Creating Topology");
  // Create a NodeContainer and add two nodes to it
  NodeContainer nodes;
  nodes.Create (2);

  // Set up a PointToPointHelper
  PointToPointHelper p2p;
  // Set the data rate attribute to 5 megabits per second 
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  // Set the delay attribute to 2 ms
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // Use the PointToPointHelper to create some PointToPointNetDevices
  // and wire PointToPointChannels between them
  NetDeviceContainer devices;
  devices = p2p.Install (nodes);

  // Now we need protocol stacks on our nodes; each node is
  // going to get an Internet stack on it.
  InternetStackHelper stack;
  stack.Install (nodes);

  // Set up an IPv4 topology helper
  Ipv4AddressHelper address;
  // Configure to begin allocating IP addresses 10.1.1.0, using the mask
  // 255.255.255.0 to define the allocatable bits. This should
  // give us 10.1.1.1, 10.1.1.2, etc.
  address.SetBase ("10.1.1.0", "255.255.255.0");
  // Now actually do the address assignment
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // Define a UDP echo server, running on port 9.
  // This line doesn't create the server; we need to assign it
  UdpEchoServerHelper echoServer (9);
  // So there is some kind of *implicit conversion* going on here
  // where the pointer to Ptr<Node> being given actually is used
  // in a constructor for an unnamed NodeContainer.
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  // So we know we're going to start the server 1 second into
  // the simulation and stop it after 10 seconds (therefore the
  // sim will run for at least 10 seconds).
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // Set up a Client application in a similar way. On this line,
  // we set the RemoteAddress and RemotePort attributes.
  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  // How many packets may the client send during the simulation?
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
  // How long to wait between packets?
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  // How large should the packet payloads be?
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  // We are installing the echo client onto the first of the
  // nodes we created.
  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  // Now we're starting the client *2 seconds* into the sim and
  // running it for 10 seconds.
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Explicit call to stop after 11 seconds.
  Simulator::Stop (Seconds (11.0));

  // ASCII tracing
  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("myfirst.tr"));
  // Wireshark-compatible tracing
  p2p.EnablePcapAll ("myfirst");


  // Global function Simulator::Run tells the system to
  // start looking through the scheduled events and executing them.
  Simulator::Run ();
  // And then stop and destroy the simulator:
  Simulator::Destroy ();
  return 0;
}
