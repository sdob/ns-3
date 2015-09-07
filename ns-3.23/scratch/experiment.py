import random
import sys

import numpy as np
import ns3

from ns.applications import MultiphaseVarclustNodeHelper
from ns.csma import CsmaHelper
from ns.core import CommandLine, DoubleValue, LOG_INFO, LogComponentEnable, Seconds, Simulator, StringValue, TimeValue, UintegerValue
from ns.internet import InternetStackHelper, Ipv4AddressHelper
from ns.network import NodeContainer, Ipv4Address, Ipv4Mask

def get_state_data(state):
    # Do a sanity check
    assert len(state) == 2
    f = open('arsenic/arsenic_%s.txt' % state)
    # Ignore the header line
    f.readline()
    measurements = []
    # Retrieve the float measurements from the file
    for line in f:
        conc = float(line.strip().split(' ')[3])
        measurements += [conc]
    return measurements

def run(data, max_packets=500, max_runtime=1000, varclust_interval=1, csma_data_rate='100Mbps', csma_delay='2ms'):
    LogComponentEnable("MultiphaseVarclustNodeApplication", LOG_INFO)

    ip_address_base = '0.0.0.0'
    ip_address_mask = '255.255.0.0'
    port_number = 9

    def set_initial_delay():
        return random.random() * varclust_interval * 2

    cmd = CommandLine()
    cmd.Parse(sys.argv)

    # Set up CSMA channel
    csma_helper = CsmaHelper()
    csma_helper.SetChannelAttribute('DataRate', StringValue(csma_data_rate))
    csma_helper.SetChannelAttribute('Delay', StringValue(csma_delay))

    # Setup IP address generation
    address_helper = Ipv4AddressHelper()
    address_helper.SetBase(Ipv4Address(ip_address_base), Ipv4Mask(ip_address_mask))

    # Create nodes
    num_nodes = len(data)
    nodes = NodeContainer()
    nodes.Create(num_nodes)

    # Install Internet stack on nodes
    stack_helper = InternetStackHelper()
    stack_helper.Install(nodes)

    # Install CSMA devices on nodes
    devices = csma_helper.Install(nodes)

    # Assign IP addresses to nodes
    interfaces = address_helper.Assign(devices)

    # Set up multiphase VarClust application on nodes
    for i in xrange(num_nodes):
        n = nodes.Get(i)
        # Set up multiphase VarClust application
        multiphase_varclust_node_helper = MultiphaseVarclustNodeHelper(port_number)
        multiphase_varclust_node_helper.SetAttribute('MaxPackets', UintegerValue(max_packets))
        multiphase_varclust_node_helper.SetAttribute('InitialEstimate', DoubleValue(data[i]))
        multiphase_varclust_node_helper.SetAttribute('Interval', TimeValue(Seconds(varclust_interval)))
        multiphase_varclust_node_helper.SetAttribute('InitialDelay', DoubleValue(set_initial_delay()))
        
        # Install multiphase VarClust on this node
        multiphase_varclust = multiphase_varclust_node_helper.Install(nodes.Get(i))
        multiphase_varclust.Get(0).SetNeighbours(interfaces)
        multiphase_varclust.Get(0).SetOwnAddress(interfaces.GetAddress(i))

        # Start at t=0, run for max_runtime seconds
        multiphase_varclust.Start(Seconds(0))
        multiphase_varclust.Stop(Seconds(500))

    # Run the simulator
    Simulator.Run()
    # At the end of the run, destroy the simulator
    Simulator.Destroy()

def run_state(state, max_packets=500, max_runtime=1000, varclust_interval=1, csma_data_rate='100Mbps', csma_delay='2ms'):
    data = get_state_data(state)
    run(data, max_packets, max_runtime, varclust_interval, csma_data_rate, csma_delay)

if __name__ == '__main__':
    state = sys.argv[1]
    max_packets = int(sys.argv[2])
    max_runtime = int(sys.argv[3])
    data = get_state_data(state)
    save_out = sys.stdout
    save_err = sys.stderr
    run_state(state, max_packets=max_packets, max_runtime=max_runtime)
