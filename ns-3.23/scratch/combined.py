#! /usr/bin/python
import random

import numpy as np
import ns3

import ns.applications
import ns.core
import ns.csma
import ns.internet
import ns.network
#import ns.p2p

from ns.core import DoubleValue, Seconds, StringValue, TimeValue, UintegerValue
from ns.network import NodeContainer

num_nodes = 3
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

stack = ns.internet.InternetStackHelper()
stack.Install(nodes)

address = ns.internet.Ipv4AddressHelper()
address.SetBase(ns.network.Ipv4Address(IP_ADDRESS_BASE), ns.network.Ipv4Mask(IP_ADDRESS_MASK))
interfaces = address.Assign(devices)

# Uniform random values
initial_values = [(random.random() - 0.5) for _ in xrange(num_nodes)]

print "Creating nodes with initial estimates."
for i in xrange(num_nodes):
    n = nodes.Get(i)
    server = ns.applications.BasicGossipServerHelper(9)
    server.SetAttribute("MaxPackets", UintegerValue(100))
    server.SetAttribute("InitialEstimate", DoubleValue(initial_values[i]))
    server.SetAttribute("Interval", TimeValue(Seconds(1)))
    server.SetAttribute("Epsilon", DoubleValue(0.00001))
    serverApps = server.Install(nodes.Get(i))
    serverApps.Get(0).SetNeighbours(interfaces)
    serverApps.Get(0).SetOwnAddress(interfaces.GetAddress(i)) # TODO: check that this is OK
    print "%s -> %s" % (interfaces.GetAddress(i), initial_values[i])
    serverApps.Start(Seconds(0))
    serverApps.Stop(Seconds(5000))

ns.core.Simulator.Run()
ns.core.Simulator.Destroy()

print "expected to converge on %s" % np.mean(initial_values)
