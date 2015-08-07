import random

import ns3

import ns.applications
import ns.core
import ns.csma
import ns.internet
import ns.network
#import ns.p2p

from ns.network import NodeContainer

num_nodes = 4
csmaDataRate = "100Mbps"
csmaDelay = "2ms"

IP_ADDRESS_BASE = "10.1.0.0"
IP_ADDRESS_MASK = "255.255.0.0"

#ns.core.LogComponentEnable('BasicGossipClientApplication', ns.core.LOG_INFO)
ns.core.LogComponentEnable('BasicGossipServerApplication', ns.core.LOG_INFO)
#ns.core.LogComponentEnable('BasicGossipClient', ns.core.LOG_INFO)
#ns.core.LogComponentEnable('Ipv4AddressHelper', ns.core.LOG_INFO)
#ns.core.LogComponentEnable('NodeContainer', ns.core.LOG_INFO)

nodes = NodeContainer()
nodes.Create(num_nodes)

csma = ns.csma.CsmaHelper()
csma.SetChannelAttribute("DataRate", ns.core.StringValue("100Mbps"))
csma.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))

devices = csma.Install(nodes)

#p2p = ns3.PointToPointHelper()
#p2p.SetChannelAttribute("DataRate", ns.core.StringValue("100Mbps"))
#p2p.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))


stack = ns.internet.InternetStackHelper()
stack.Install(nodes)

address = ns.internet.Ipv4AddressHelper()
address.SetBase(ns.network.Ipv4Address(IP_ADDRESS_BASE), ns.network.Ipv4Mask(IP_ADDRESS_MASK))
interfaces = address.Assign(devices)

print "Creating nodes."
for i in xrange(num_nodes):
    n = nodes.Get(i)
    server = ns.applications.BasicGossipServerHelper(9)
    server.SetAttribute("MaxPackets", ns.core.UintegerValue(10))
    server.SetAttribute("InitialEstimate", ns.core.DoubleValue(random.random()))
    serverApps = server.Install(nodes.Get(i))
    serverApps.Get(0).SetNeighbours(interfaces)
    serverApps.Get(0).SetOwnAddress(interfaces.GetAddress(i)) # TODO: check that this is OK
    #serverApps.Start(ns.core.Seconds(0))
    #serverApps.Stop(ns.core.Seconds(100))

# 
#def scheduled_callback():
    #print "callback at %.3ss" % (ns.core.Simulator.Now().GetMilliSeconds() / 1000.0)
#ns.core.Simulator.Schedule(ns.core.Seconds(2), scheduled_callback)

ns.network.Packet.EnablePrinting()
ns.core.Simulator.Run()
ns.core.Simulator.Destroy()
