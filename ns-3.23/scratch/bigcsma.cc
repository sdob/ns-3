#include <iostream>

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

float average (int val1, int val2) {
  return float (val1 + val2) / 2;
}

int main (int argc, char *argv[]) {
  // TODO: create an array of values
  // TODO: create an array of Boolean flags
  std::cout << "hi there" << std::endl;
  std::cout << "(5 + 6) / 2 = " << average(5, 6) << std::endl;

  uint32_t nCsma = 100;
  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of CSMA nodes", nCsma);
  cmd.Parse (argc, argv);

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer csmaNodes;
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  InternetStackHelper stack;
  stack.Install (csmaNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma - 1));
  serverApps.Start (Seconds (0.0));
  //serverApps.Stop (Seconds (nCsma));

  for (uint32_t i = 0; i < nCsma-1; i++) {
    UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma - 1), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue(Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps = echoClient.Install (csmaNodes.Get (i));
    clientApps.Start (Seconds (1));
    //clientApps.Stop (Seconds (10));
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
