#include <sstream>
#include <math.h>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <map>

#include "boost/tuple/tuple.hpp"

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/random-variable-stream.h"

#include "multiphase-varclust-node.h"

namespace ns3 {
  NS_LOG_COMPONENT_DEFINE ("MultiphaseVarclustNodeApplication");
  NS_OBJECT_ENSURE_REGISTERED (MultiphaseVarclustNode);

  TypeId MultiphaseVarclustNode::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::MultiphaseVarclustNode")
      .SetParent<Application> ()
      .AddConstructor<MultiphaseVarclustNode> ()
      .AddAttribute ("Port", "Port on which we listen for incoming packets",
          UintegerValue (9),
          MakeUintegerAccessor (&MultiphaseVarclustNode::m_port),
          MakeUintegerChecker<uint16_t> ())
      .AddAttribute ("EpochLength", "The length (in number of packets sent) of an epoch of gossip",
          UintegerValue (20),
          MakeUintegerAccessor (&MultiphaseVarclustNode::m_length_of_epoch),
          MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("MaxPackets", "The maximum number of packets the application will send; if -1 then should run until convergence",
          UintegerValue (100),
          MakeUintegerAccessor (&MultiphaseVarclustNode::m_count),
          MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("InitialDelay", "The offset from 02 when the node should start sending",
          DoubleValue (0.0),
          MakeDoubleAccessor (&MultiphaseVarclustNode::m_initial_delay),
          MakeDoubleChecker<double> ())
      .AddAttribute ("InitialEstimate", "The initial estimate the node is holding",
          DoubleValue (0.0),
          MakeDoubleAccessor (&MultiphaseVarclustNode::m_initial_measurement),
          MakeDoubleChecker<double> ())
      .AddAttribute ("Interval", "The time to wait between packets",
          TimeValue (Seconds (1.0)),
          MakeTimeAccessor (&MultiphaseVarclustNode::m_interval),
          MakeTimeChecker ())
      .AddAttribute ("Epsilon", "The minimum change in estimate for a node to continue gossip",
          DoubleValue (pow (10, -4)),
          MakeDoubleAccessor (&MultiphaseVarclustNode::m_epsilon),
          MakeDoubleChecker<double> ())
      ;
    return tid;
  }


  MultiphaseVarclustNode::MultiphaseVarclustNode () {
    NS_LOG_FUNCTION (this);
    m_sendEvent = EventId ();
    m_sent = 0;
  }


  MultiphaseVarclustNode::~MultiphaseVarclustNode () {
    NS_LOG_FUNCTION (this);
    m_socket_active = 0;
    m_socket_passive = 0;
  }


  void MultiphaseVarclustNode::DoDispose (void) {
    NS_LOG_FUNCTION (this);
    Application::DoDispose ();
  }


  void MultiphaseVarclustNode::ScheduleTransmit (Time dt) {
    NS_LOG_FUNCTION (this << dt);
    m_sendEvent = Simulator::Schedule (dt, &MultiphaseVarclustNode::Send, this);
  }


  Ipv4Address MultiphaseVarclustNode::GetNeighbour (void) {
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    x->SetAttribute ("Min", DoubleValue (0));
    x->SetAttribute ("Max", DoubleValue(m_interfaces.GetN() - 1));
    int index = x->GetInteger ();
    while (
        m_interfaces.GetAddress (index) == m_own_address || // Don't contact yourself
        !m_connectivity_map[m_interfaces.GetAddress (index)] // Don't contact nodes from which you've cut yourself off
        ) {
      index = x->GetInteger ();
    }
    return m_interfaces.GetAddress (index);
  }

  void MultiphaseVarclustNode::SetNeighbours (Ipv4InterfaceContainer interfaces) {
    NS_LOG_FUNCTION (this);
    m_interfaces = interfaces;
    // Initialize connectivity map (everything is initially true)
    // and the connectivity decision map (everything is initially false)
    for (uint32_t i = 0; i < interfaces.GetN (); ++i) {
      m_connectivity_map[interfaces.GetAddress (i)] = true;
      m_connectivity_decisions[interfaces.GetAddress (i)] = false;
    }
  }


  void MultiphaseVarclustNode::SetOwnAddress (Ipv4Address address) {
    NS_LOG_FUNCTION (this);
    m_own_address = address;
  }


  void MultiphaseVarclustNode::StartApplication (void) {
    NS_LOG_FUNCTION (this);

    // Set epoch to 0
    m_current_epoch = 0;
    // Initialize the count of messages this epoch
    m_messages_this_epoch = 0;
    // We haven't changed in this epoch yet (because we're just beginning)
    m_changed_this_epoch = false;

    // Set previous estimates initially equal to the initial estimates
    m_estimate_w = m_initial_measurement;
    m_old_estimate_w = m_estimate_w;
    m_estimate_w2 = m_estimate_w * m_estimate_w;
    m_old_estimate_w2 = m_estimate_w2;

    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

    // Set up the socket for the passive thread
    m_socket_passive = Socket::CreateSocket (GetNode (), tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
    m_socket_passive->Bind (local);
    // XXX: Magic incantation here
    if (addressUtils::IsMulticast (m_local)) {
      Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket_passive);
      if (udpSocket) {
        udpSocket->MulticastJoinGroup (0, m_local);
      } else {
        NS_FATAL_ERROR ("Error: Failed to join multicast group");
      }
    } 
    m_socket_passive->SetRecvCallback (MakeCallback (&MultiphaseVarclustNode::HandlePassiveRead, this));

    // Set up the socket for the active thread
    m_socket_active = Socket::CreateSocket (GetNode (), tid);
    m_socket_active->Bind ();
    m_socket_active->SetRecvCallback (MakeCallback (&MultiphaseVarclustNode::HandleActiveRead, this));

    // Schedule the first transmission
    ScheduleTransmit (Seconds (0. + m_initial_delay));

    // Log initialization
    NS_LOG_INFO (
        Simulator::Now ().GetSeconds ()
        << " " << "INIT"
        << " " << Ipv4Address::ConvertFrom (m_own_address)
        << " " << m_initial_measurement
        );
  }


  void MultiphaseVarclustNode::StopApplication (void) {
    NS_LOG_FUNCTION (this);
    // Close the passive socket
    m_socket_passive->Close ();
    m_socket_passive->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    // Close the active socket
    m_socket_active->Close ();
    m_socket_active->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    // Cancel any pending simulator event
    Simulator::Cancel (m_sendEvent);
  }


  void MultiphaseVarclustNode::Send (void) {
    NS_LOG_FUNCTION (this);

    // TODO: check whether the previous update changed our estimates enough
    // that we should continue gossiping.

    // Check whether this node has a neighbour that it can send to
    bool hasANeighbour = false;
    for (uint32_t i = 0; i < m_interfaces.GetN(); i++) {
      if (m_connectivity_map[m_interfaces.GetAddress (i)]) {
        hasANeighbour = true;
        break;
      }
    }
    if (!hasANeighbour) {
      ScheduleTransmit (m_interval);
      return;
    }
    // Choose a random neighbour from the vector of interfaces and connect
    Ipv4Address dest = GetNeighbour ();
    m_socket_active->Connect (InetSocketAddress (dest, m_port));
    // Create and send a packet with the current estimates as a payload
    Ptr<Packet> p = MakePacket (m_current_epoch, m_initial_measurement, m_estimate_w, m_estimate_w2);
    m_socket_active->Send (p);

    NS_LOG_INFO(
        Simulator::Now ().GetSeconds () // Time
        << " " << "ASEND"
        << " " << Ipv4Address::ConvertFrom (m_own_address)
        << " " << dest
        << " " << m_current_epoch
        << " " << std::setprecision(12) << m_initial_measurement // this node's initial measurement
        << " " << std::setprecision(12) << m_estimate_w // this node's m_estimate_w
        << " " << std::setprecision(12) << m_estimate_w2 // 
        );

    // Increment the number of messages sent in total and during this epoch;
    // if we've reached the limit, then increment the epoch. 
    ++m_sent;
    if (m_sent < m_count && m_count > 0) {
      ScheduleTransmit (m_interval);
    }
    ++m_messages_this_epoch;

    if (m_messages_this_epoch >= m_length_of_epoch) {
      ++m_current_epoch;
      StartEpoch (m_changed_this_epoch);
    }

  }


  void MultiphaseVarclustNode::HandleActiveRead (Ptr<Socket> socket) {
    NS_LOG_FUNCTION (this);

    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom (from))) {
      // Extract epoch number, initial measurement, and estimates from received packet payload
      boost::tuple<int, double, double, double> data = ExtractPayload (packet);

      Ipv4Address sender = InetSocketAddress::ConvertFrom (from).GetIpv4 ();

      // Log received packet data
      NS_LOG_INFO (
          Simulator::Now ().GetSeconds () // Time
          << " " << "ARECV" // Receiving on active thread
          << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
          << " " << sender // sender's address
          << " " << boost::get<0>(data) // sender's epoch number
          << " " << std::setprecision(12) << boost::get<1>(data) // sender's initial_measurement
          << " " << std::setprecision(12) << boost::get<2>(data) // sender's m_estimate_w
          << " " << std::setprecision(12) << boost::get<3>(data) // sender's m_estimate_w2
          );

      // We might need to update the epoch if the sender is ahead of us
      UpdateEpoch (boost::get<0> (data));

      // Update estimates
      Update (data, sender);
    }
  }


  void MultiphaseVarclustNode::HandlePassiveRead (Ptr<Socket> socket) {
    NS_LOG_FUNCTION (this);

    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom (from))) {
      // Extract epoch number, initial measurement, and estimates from received packet payload
      boost::tuple<int, double, double, double> data = ExtractPayload (packet);
      // Log received packet data
      NS_LOG_INFO (
          Simulator::Now ().GetSeconds () // Time
          << " " << "PRECV" // Receiving on passive thread
          << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
          << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () // sender's address
          << " " << boost::get <0>(data) // sender's epoch number
          << " " << std::setprecision(12) << boost::get<1> (data) // sender's m_estimate_w
          << " " << std::setprecision(12) << boost::get<2> (data) // sender's m_estimate_w
          << " " << std::setprecision(12) << boost::get<3> (data) // sender's m_estimate_w2
          );

      // We might need to update the epoch if the sending node is ahead of us
      UpdateEpoch (boost::get<0> (data));

      // Put the current estimates into a packet and send it 
      Ptr<Packet> p = MakePacket (m_current_epoch, m_initial_measurement, m_estimate_w, m_estimate_w2);
      socket->SendTo (p, 0, from);
      NS_LOG_INFO (
          Simulator::Now ().GetSeconds () // Time
          << " " << "PSEND" // Responding on passive thread
          << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
          << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () // sender's address
          << " " << m_current_epoch // this node's epoch number
          << " " << std::setprecision(12) << m_initial_measurement // this node's initial measurement
          << " " << std::setprecision(12) << m_estimate_w // this node's m_estimate_w
          << " " << std::setprecision(12) << m_estimate_w2 // 
          );

      // Update held estimates
      Update (data, InetSocketAddress::ConvertFrom (from).GetIpv4 ());
    }
  }


  Ptr<Packet> MultiphaseVarclustNode::MakePacket (int epoch, double initial_measurement, double estimate_w, double estimate_w2) {
    NS_LOG_FUNCTION (this);
    // Pack the epoch number, initial measurement, and estimates into a string
    std::ostringstream msg;
    char buffer [50];
    sprintf (buffer, "%d|%.10f|%.10f|%.10f", epoch, initial_measurement, estimate_w, estimate_w2);
    msg << buffer << '\0';
    // Create and return a packet
    return Create<Packet> ((uint8_t*) msg.str ().c_str (), msg.str ().length ());
  }


  boost::tuple<int, double, double, double> MultiphaseVarclustNode::ExtractPayload (Ptr<Packet> packet) {
    NS_LOG_FUNCTION (this);

    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();
    uint8_t *buffer = new uint8_t[packet->GetSize ()];
    packet->CopyData (buffer, packet->GetSize ());
    // Retrieve epoch number, initial measurement, and estimates from the packet
    int i;
    char *p;
    char *array[4];
    i = 0;
    p = strtok ((char*) buffer, "|");
    while (p != NULL) {
      array[i++] = p;
      p = strtok (NULL, "|");
    }
    // Return a tuple
    return boost::tuple<int, double, double, double> (atoi(array[0]), atof(array[1]), atof(array[2]), atof(array[3]));
  }


  void MultiphaseVarclustNode::Update (boost::tuple<int, double, double, double> data, Ipv4Address from) {
    NS_LOG_FUNCTION (this);
    double initial_measurement = boost::get<1> (data);
    double estimate_w = boost::get<2> (data);
    double estimate_w2 = boost::get<3> (data);

    // Store this neighbour's measurement
    m_neighbour_measurements[from] = initial_measurement;

    // Store old estimates
    m_old_estimate_w = m_estimate_w;
    m_old_estimate_w2 = m_estimate_w2;

    // Update new estimates
    m_estimate_w = (estimate_w + m_estimate_w) / 2;
    m_estimate_w2 = (estimate_w2 + m_estimate_w2) / 2;

    // Update variance estimate
    m_variance = m_estimate_w2 - (m_estimate_w * m_estimate_w);

    // Log the update
    NS_LOG_INFO (
        Simulator::Now ().GetSeconds () // Time
        << " " << "UPDAT" // Updating estimates
        << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
        << " " << m_current_epoch
        << " " << std::setprecision(12) << m_old_estimate_w
        << " " << std::setprecision(12) << m_estimate_w
        << " " << std::setprecision(12) << m_old_estimate_w2
        << " " << std::setprecision(12) << m_estimate_w2
        << " " << std::setprecision(12) << m_variance
        );

    // Update the connectivity map
    UpdateConnectivityDecisions ();
  }


  void MultiphaseVarclustNode::UpdateConnectivityDecisions (void) {
    NS_LOG_FUNCTION(this);
    // Iterate over the stored neighbours' measurements and make a connectivity decision
    for (std::map<Ipv4Address, double>::iterator it = m_neighbour_measurements.begin(); it != m_neighbour_measurements.end(); ++it) {
      Ipv4Address neighbour = it->first;
      double y = it->second;
      // We'll want to know if a change has been made
      bool old_connectivity_decision = m_connectivity_decisions[neighbour];
      if (fabs(m_initial_measurement - y) <= pow(m_variance, 0.5)) {
        m_connectivity_decisions[neighbour] = true;
      } else {
        m_connectivity_decisions[neighbour] = false;
      }
      // If this node's connectivity map has changed w.r.t. a neighbour, log the change
      // (we will also need to know this in order to decide whether to continue to
      // another epoch)
      if (m_connectivity_decisions[neighbour] != old_connectivity_decision) {
        m_changed_this_epoch = true;
        NS_LOG_INFO (
            Simulator::Now ().GetSeconds () // Time
            << " " << "CHANGE" // Changing connectivity decision
            << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
            << " " << neighbour // sender's address
            << " " << old_connectivity_decision
            << " " << m_connectivity_decisions[neighbour]
            );
      }
    }
  }


  void MultiphaseVarclustNode::UpdateEpoch (int epoch) {
    NS_LOG_FUNCTION (this);

    if (epoch > m_current_epoch) {
      m_current_epoch = epoch;
      // Handle the beginning of a new epoch
      StartEpoch (m_changed_this_epoch);
    }
  }


  void MultiphaseVarclustNode::StartEpoch (bool changed_this_epoch) {
    // Reset the counter of messages sent this epoch
    m_messages_this_epoch = 0;
    // Set the current connectivity map to the state of the connectivity
    // decision map at the end of the last epoch
    for (uint32_t i = 0; i < m_interfaces.GetN(); i++) {
      m_connectivity_map[m_interfaces.GetAddress (i)] = m_connectivity_decisions[m_interfaces.GetAddress (i)];
    }
    // Reset estimates
    
    // Set previous estimates initially equal to the initial estimates
    m_estimate_w = m_initial_measurement;
    m_old_estimate_w = m_estimate_w;
    m_estimate_w2 = m_estimate_w * m_estimate_w;
    m_old_estimate_w2 = m_estimate_w2;

    // TODO: handle expected behaviour when an epoch changes. We will
    // need to check whether the connectivity map changed during the last
    // epoch, and flag to continue gossip accordingly
    // Update epoch number
    NS_LOG_INFO (
        Simulator::Now ().GetSeconds () // Time
        << " " << "PHASE" // Changing epoch
        << " " << Ipv4Address::ConvertFrom (m_own_address)
        << " " << m_current_epoch
        );
  }
}
