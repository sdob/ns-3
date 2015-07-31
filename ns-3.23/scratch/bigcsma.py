import ns.applications
import ns.core
import ns.csma
import ns.internet
import ns.network

nCsma = 3
csmaDataRate = "100Mbps"
csmaDelay = "2ms"

IP_ADDRESS_BASE = "10.1.0.0"
IP_ADDRESS_MASK = "255.255.0.0"

ns.core.LogComponentEnable('UdpClient', ns.core.LOG_LEVEL_INFO)
#ns.core.LogComponentEnable('UdpEchoServerApplication', ns.core.LOG_LEVEL_INFO)

nodes = ns.network.NodeContainer()
nodes.Create(nCsma)

csma = ns.csma.CsmaHelper()
csma.SetChannelAttribute("DataRate", ns.core.StringValue("100Mbps"))
csma.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))

devices = csma.Install(nodes)

stack = ns.internet.InternetStackHelper()
stack.Install(nodes)

address = ns.internet.Ipv4AddressHelper()
address.SetBase(ns.network.Ipv4Address(IP_ADDRESS_BASE), ns.network.Ipv4Mask(IP_ADDRESS_MASK))

interfaces = address.Assign(devices)

print "creating server apps"

server = ns.applications.UdpEchoServerHelper(9)
#for i in xrange(nCsma):
serverApps = server.Install(nodes)

serverApps.Start(ns.core.Seconds(0))


print "creating client apps"
clientApps = ns.network.ApplicationContainer()
for i in xrange(nCsma):
    echoClient = ns.applications.UdpClientHelper(interfaces.GetAddress(i), 9)
    echoClient.SetAttribute("MaxPackets", ns.core.UintegerValue(2))
    echoClient.SetAttribute("Interval", ns.core.TimeValue(ns.core.Seconds(0.0)))
    echoClient.SetAttribute("PacketSize", ns.core.UintegerValue(1024))
    node_container = ns.network.NodeContainer()
    node_container.Add(nodes.Get(i))
    clientApps.Add(echoClient.Install(node_container))
    #print clientApps
print dir(clientApps.Get(0))

print "starting client apps"
clientApps.Start(ns.core.Seconds(0))
clientApps.Stop(ns.core.Seconds(100))

def scheduled_callback():
    print "callback at %.3ss" % (ns.core.Simulator.Now().GetMilliSeconds() / 1000.0)

ns.core.Simulator.Schedule(ns.core.Seconds(2), scheduled_callback)

ns.core.Simulator.Run()
ns.core.Simulator.Destroy()
