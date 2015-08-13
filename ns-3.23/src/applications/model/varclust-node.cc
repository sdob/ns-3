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

#include "varclust-node.h"


namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("VarclustNodeApplication");
  NS_OBJECT_ENSURE_REGISTERED (VarclustNode);

  TypeId VarclustNode::GetTypeId (void) {
    static TypeId tid = TypeId ("ns3::VarclustNode")
      .SetParent<Application> ()
      .AddConstructor<VarclustNode> ()
      .AddAttribute ("Port", "Port on which we listen for incoming packets.",
          UintegerValue (9),
          MakeUintegerAccessor (&VarclustNode::m_port),
          MakeUintegerChecker<uint16_t> ())
      .AddAttribute ("MaxPackets", "The maximum number of packets the application will send",
          UintegerValue (100),
          MakeUintegerAccessor (&VarclustNode::m_count),
          MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("InitialDelay", "The initial estimate the node is holding",
          DoubleValue (0.0),
          MakeDoubleAccessor (&VarclustNode::m_initial_delay),
          MakeDoubleChecker<double> ())
      .AddAttribute ("Interval", "The time to wait between packets",
          TimeValue (Seconds (1.0)),
          MakeTimeAccessor (&VarclustNode::m_interval),
          MakeTimeChecker ())
      .AddAttribute ("InitialEstimate", "The initial estimate the node is holding",
          DoubleValue (0.0),
          MakeDoubleAccessor (&VarclustNode::m_initial_measurement),
          MakeDoubleChecker<double> ())
      .AddAttribute ("Epsilon", "The minimum change in estimate for a node to continue gossip",
          DoubleValue (pow(10, -4)),
          MakeDoubleAccessor (&VarclustNode::m_epsilon),
          MakeDoubleChecker<double> ())
      ;
    return tid;
  }


  VarclustNode::VarclustNode () {
    NS_LOG_FUNCTION (this);
    m_sendEvent = EventId ();
    m_sent = 0;
  }


  VarclustNode::~VarclustNode () {
    NS_LOG_FUNCTION (this);
    m_socket_active = 0;
    m_socket_passive = 0;
  }


  void VarclustNode::DoDispose (void) {
    NS_LOG_FUNCTION (this);
    Application::DoDispose ();
  }


  void VarclustNode::ScheduleTransmit (Time dt) {
    NS_LOG_FUNCTION (this << dt);
    m_sendEvent = Simulator::Schedule (dt, &VarclustNode::Send, this);
  }


  void VarclustNode::SetNeighbours (Ipv4InterfaceContainer interfaces) {
    NS_LOG_FUNCTION (this);
    m_interfaces = interfaces;
    for (uint32_t i = 0; i < interfaces.GetN(); ++i) {
      m_connectivity_map[interfaces.GetAddress(i)] = false;
    }
  }


  void VarclustNode::SetOwnAddress (Ipv4Address address) {
    NS_LOG_FUNCTION (this << address);
    m_own_address = address;
  }


  void VarclustNode::StartApplication (void) {
    NS_LOG_FUNCTION (this);

    // Set the previous estimates equal to the initial estimates
    m_estimate_w = m_initial_measurement;
    m_old_estimate_w = m_estimate_w;
    m_estimate_w2 = m_estimate_w * m_estimate_w;
    m_old_estimate_w2 = m_estimate_w2;

    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

    // Set up the socket for passive listening and responding
    m_socket_passive = Socket::CreateSocket (GetNode (), tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
    m_socket_passive->Bind (local);
    // XXX: Magic incantation here
    if (addressUtils::IsMulticast (m_local)) {
      Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket_passive);
      if (udpSocket) {
        // equivalent to setsockopt (MCAST_JOIN_GROUP)
        udpSocket->MulticastJoinGroup (0, m_local);
      } else {
        NS_FATAL_ERROR ("Error: Failed to join multicast group");
      }
    }
    m_socket_passive->SetRecvCallback (MakeCallback (&VarclustNode::HandlePassiveRead, this));

    // Set up the socket for active sending and receiving responses
    m_socket_active = Socket::CreateSocket (GetNode (), tid);
    m_socket_active->Bind ();
    m_socket_active->SetRecvCallback (MakeCallback (&VarclustNode::HandleActiveRead, this));

    // Schedule the first transmission for 0s + random delay
    ScheduleTransmit (Seconds (0. + m_initial_delay));

    // Log initialization
    NS_LOG_INFO(
        Simulator::Now ().GetSeconds()
        << " " << "INIT"
        << " " << Ipv4Address::ConvertFrom (m_own_address)
        << " " << m_initial_measurement
        );
  }


  void VarclustNode::StopApplication () {
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


  void VarclustNode::Send (void) {
    // If we've started gossiping and there's been no change, then re-schedule
    // a send event for the next cycle in case our estimates change
    if (m_sent > 0
        && fabs(m_estimate_w - m_old_estimate_w) < m_epsilon
        && fabs(m_estimate_w2 - m_old_estimate_w2) < m_epsilon) {
      ScheduleTransmit (m_interval);
      return;
    }

    // Choose a random neighbour from the vector of interfaces. If there are
    // fewer than two nodes, this is going to loop forever, so don't do that.
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    x->SetAttribute ("Min", DoubleValue (0));
    x->SetAttribute ("Max", DoubleValue(m_interfaces.GetN() - 1));
    int index = x->GetInteger ();
    while (m_interfaces.GetAddress (index) == m_own_address) {
      index = x->GetInteger ();
    }
    Ipv4Address dest = m_interfaces.GetAddress (index);

    // Create a packet with the current estimates as a payload
    Ptr<Packet> p = MakePacket(m_initial_measurement, m_estimate_w, m_estimate_w2);

    // Connect to the neighbour and send the packet
    m_socket_active->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (dest), m_port));
    m_socket_active->Send (p);

    ++m_sent;
    if (m_sent < m_count) {
      ScheduleTransmit (m_interval);
    }

    NS_LOG_INFO (
        Simulator:: Now ().GetSeconds () // Time
        << " " << "ASEND" // Initiating gossip
        << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
        << " " << Ipv4Address::ConvertFrom (dest) // recipient's address
        << " " << std::setprecision(12) << m_estimate_w // this node's m_estimate_w
        << " " << std::setprecision(12) << m_estimate_w2 // this node's m_estimate_w2
        );
  }


  void VarclustNode::HandleActiveRead (Ptr<Socket> socket) {
    NS_LOG_FUNCTION (this << socket);

    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom (from))) {
      // Extract an array of doubles from the packet payload
      boost::tuple<double, double, double> data = ExtractPayload (packet);
      NS_LOG_INFO (
          Simulator::Now ().GetSeconds () // Time
          << " " << "ARECV" // Receiving on active thread
          << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
          << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () // sender's address
          << " " << std::setprecision(12) << boost::get<0>(data) // sender's m_estimate_w
          << " " << std::setprecision(12) << boost::get<1>(data) // sender's m_estimate_w
          << " " << std::setprecision(12) << boost::get<2>(data) // sender's m_estimate_w2
          );

      // Update held estimates
      Update (data, InetSocketAddress::ConvertFrom (from).GetIpv4 ());
    }
  }

  void VarclustNode::HandlePassiveRead (Ptr<Socket> socket) {
    NS_LOG_FUNCTION (this << socket);

    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom (from))) {
      // Extract an array of doubles from the packet payload
      boost::tuple<double, double, double> data = ExtractPayload (packet);
      NS_LOG_INFO (
          Simulator::Now ().GetSeconds () // Time
          << " " << "PRECV" // Receiving on passive thread
          << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
          << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () // sender's address
          << " " << std::setprecision(12) << boost::get<0>(data) // sender's m_estimate_w
          << " " << std::setprecision(12) << boost::get<1>(data) // sender's m_estimate_w
          << " " << std::setprecision(12) << boost::get<2>(data) // sender's m_estimate_w2
          );

      // Put the current estimates into a packet and send it 
      Ptr<Packet> p = MakePacket (m_initial_measurement, m_estimate_w, m_estimate_w2);
      socket->SendTo (p, 0, from);
      NS_LOG_INFO (
          Simulator::Now ().GetSeconds () // Time
          << " " << "PSEND" // Responding on passive thread
          << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
          << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () // sender's address
          << " " << std::setprecision(12) << m_initial_measurement // this node's initial measurement
          << " " << std::setprecision(12) << m_estimate_w // this node's m_estimate_w
          << " " << std::setprecision(12) << m_estimate_w2 // 
          );

      // Update held estimates
      Update (data, InetSocketAddress::ConvertFrom (from).GetIpv4 ());
    }
  }


  Ptr<Packet> VarclustNode::MakePacket (double initial_measurement, double estimate_w, double estimate_w2) {
    NS_LOG_FUNCTION (this << estimate_w << estimate_w2);
    // Pack the two values, delimited by '|', into a string
    std::ostringstream msg;
    char buffer [50];
    sprintf (buffer, "%.10f|%.10f|%.10f", initial_measurement, estimate_w, estimate_w2);
    msg << buffer << '\0';
    // Create and return a packet
    return Create<Packet> ((uint8_t*) msg.str().c_str(), msg.str().length());
  }


  boost::tuple<double, double, double> VarclustNode::ExtractPayload (Ptr<Packet> packet) {
    NS_LOG_FUNCTION (this << packet);
    // Extract string from packet contents
    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();
    uint8_t *buffer = new uint8_t[packet->GetSize()];
    packet->CopyData (buffer, packet->GetSize());
    // Retrieve estimates of w and w^2 from the packet
    // XXX: Magic incantations here 
    int i;
    char *p;
    char *array[3];
    i = 0;
    p = strtok ((char*) buffer, "|");
    while (p != NULL) {
      array[i++] = p;
      p = strtok (NULL, "|");
    }

    return boost::tuple<double, double, double> (atof(array[0]), atof(array[1]), atof(array[2]));
  }


  void VarclustNode::Update (boost::tuple<double, double, double> data, Ipv4Address from) {
    NS_LOG_FUNCTION (this);

    double initial_measurement = boost::get<0>(data);
    double estimate_w = boost::get<1>(data);
    double estimate_w2 = boost::get<2>(data);

    // Store old estimates
    m_old_estimate_w = m_estimate_w;
    m_old_estimate_w2 = m_estimate_w2;

    // Update new estimates
    m_estimate_w = (estimate_w + m_estimate_w) / 2;
    m_estimate_w2 = (estimate_w2 + m_estimate_w2) / 2;

    // Update variance estimate
    m_variance = m_estimate_w2 - (m_estimate_w * m_estimate_w);

    NS_LOG_INFO (
        Simulator::Now ().GetSeconds () // Time
        << " " << "UPDAT" // Updating estimates
        << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
        << " " << std::setprecision(12) << m_old_estimate_w
        << " " << std::setprecision(12) << m_estimate_w
        << " " << std::setprecision(12) << m_old_estimate_w2
        << " " << std::setprecision(12) << m_estimate_w2
        << " " << std::setprecision(12) << m_variance
        );

    bool old_connectivity_decision = m_connectivity_map[from];
    // update connectivity map
    if (fabs(m_initial_measurement - initial_measurement) <= pow(m_variance, 0.5)) {
      m_connectivity_map[from] = true;
    } else {
      m_connectivity_map[from] = false;
    }

    if (m_connectivity_map[from] != old_connectivity_decision) {
      NS_LOG_INFO (
          Simulator::Now ().GetSeconds () // Time
          << " " << "CHANGE" // Changing connectivity decision
          << " " << Ipv4Address::ConvertFrom (m_own_address) // this node's address
          << " " << from // sender's address
          << " " << old_connectivity_decision
          << " " << m_connectivity_map[from]
          );
    }

  }
}
