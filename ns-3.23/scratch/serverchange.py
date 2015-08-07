import ns3
import ns.applications
import ns.core
import ns.csma
import ns.internet

from ns.core import LOG_INFO as LOG_LEVEL_ALL, LogComponentEnable, \
        StringValue, TimeValue, UintegerValue, Seconds, Simulator

from ns.network import NodeContainer

LogComponentEnable("RequestResponseClientApplication", LOG_LEVEL_ALL)
#LogComponentEnable("RequestResponseServerApplication", LOG_LEVEL_ALL)

print "Create nodes."
n = NodeContainer()
n.Create(4)

internet = ns.internet.InternetStackHelper()
internet.Install(n)

print "Create channels."
csma = ns.csma.CsmaHelper()
csma.SetChannelAttribute("DataRate", StringValue("5Mbps"))
csma.SetChannelAttribute("Delay", StringValue("2ms"))
csma.SetDeviceAttribute("Mtu", UintegerValue(1400))
d = csma.Install(n)

print "Assign IP Addresses."
ipv4 = ns.internet.Ipv4AddressHelper()
ipv4.SetBase(ns.network.Ipv4Address("10.1.1.0"), ns.network.Ipv4Mask("255.255.255.0"))
i = ipv4.Assign(d)
server1Address = ns.network.Address(i.GetAddress(1))
server2Address = ns.network.Address(i.GetAddress(2))

print "Create applications."
port = 9
server = ns.applications.RequestResponseServerHelper(port)
responseSize = 1024
server.SetAttribute("PacketSize", UintegerValue(responseSize))
server1Apps = server.Install(n.Get(1))
server1Apps.Start(Seconds(1.0))
server1Apps.Stop(Seconds(10.0))

server2Apps = server.Install(n.Get(2))
server2Apps.Start(Seconds(1.0))
server2Apps.Stop(Seconds(10.0))

packetSize = 1
maxPacketCount = 4
interPacketInterval = Seconds(2.)
client = ns.applications.RequestResponseClientHelper(server1Address, port)
client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount))
client.SetAttribute("Interval", TimeValue(interPacketInterval))
client.SetAttribute("PacketSize", UintegerValue(packetSize))
clientApps = client.Install(n.Get(0))
clientApps.Start(Seconds(2.0))
clientApps.Stop(Seconds(10.0))

print dir(clientApps.Get(0))

# Schedule a callback
def handler():
    print "Changing server."
    #print clientApps.Get(0).socket
    clientApps.Get(0).SetDataSize(2)
    clientApps.Get(0).SetRemote(i.GetAddress(2), port)

Simulator.Schedule(Seconds(3), handler)

print "Run simulation."
Simulator.Run()
Simulator.Destroy()
print "Done."
