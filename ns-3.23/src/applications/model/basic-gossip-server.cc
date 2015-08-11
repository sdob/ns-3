/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

#include "basic-gossip-server.h"

#include <sstream>
#include <math.h>
#include <cmath>
#include <iomanip>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BasicGossipServerApplication");

NS_OBJECT_ENSURE_REGISTERED (BasicGossipServer);

TypeId
BasicGossipServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BasicGossipServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<BasicGossipServer> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&BasicGossipServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MaxPackets", "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&BasicGossipServer::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&BasicGossipServer::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("InitialEstimate", "The initial estimate the node is holding",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&BasicGossipServer::m_estimate),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Epsilon", "The minimum change in estimate for a node to continue gossip",
                   DoubleValue (pow(10, -4)),
                   MakeDoubleAccessor (&BasicGossipServer::m_epsilon),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

BasicGossipServer::BasicGossipServer ()
{
  NS_LOG_FUNCTION (this);
  m_sendEvent = EventId ();
  m_sent = 0; // number of packets sent
}


BasicGossipServer::~BasicGossipServer()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socket_send = 0;
}


void
BasicGossipServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}



void
BasicGossipServer::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &BasicGossipServer::Send, this);
}


void
BasicGossipServer::SetNeighbours (Ipv4InterfaceContainer interfaces)
{
  NS_LOG_FUNCTION (this);
  m_interfaces = interfaces;
}


void
BasicGossipServer::SetOwnAddress (Address address)
{
  NS_LOG_FUNCTION(this << address);
  m_own_address = address;
}


void 
BasicGossipServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  m_old_estimate = m_estimate;


  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  if (m_socket == 0)
    {
      // Create the socket that will listen for and respond to traffic sent by other nodes
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: Failed to join multicast group");
            }
        }

    }

    // Create the socket that will initiate gossip and listen for responses
    m_socket_send = Socket::CreateSocket (GetNode (), tid);
    m_socket->Bind ();

  // Handle response on the receiving socket
  m_socket->SetRecvCallback (MakeCallback (&BasicGossipServer::HandleRead, this));

  // Handle response on the sending socket
  m_socket_send->SetRecvCallback (MakeCallback (&BasicGossipServer::HandleReadWithoutResponse, this));

  // Schedule a transmission
  ScheduleTransmit (Seconds (0.));
}

void 
BasicGossipServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  if (m_socket_send != 0)
    {
      m_socket_send->Close ();
      m_socket_send->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  Simulator::Cancel (m_sendEvent);
}


void
BasicGossipServer::Send (void)
{
  // If we've started gossip and there's been no change, then re-schedule a send event for the next cycle
  if (m_sent > 0 && fabs(m_estimate - m_old_estimate) < m_epsilon) {
    ScheduleTransmit (m_interval);
    return;
  }

  // Otherwise, choose a neighbour, create a packet containing the current estimate,
  // and send the packet to the neighbour

  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  // Choose a random neighbour from the interfaces vector. If there are fewer than
  // two nodes, then this will loop forever, do don't do that.
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue(0));
  x->SetAttribute ("Max", DoubleValue(m_interfaces.GetN() - 1));
  int index = x->GetInteger ();
  // Loop until we find a random neighbour that isn't this node
  while (m_interfaces.GetAddress(index) == m_own_address) {
    index = x->GetInteger ();
  }
  Address dest = m_interfaces.GetAddress(index);

  // Create a packet with the current estimate as payload
  std::ostringstream msg;
  msg << std::setprecision(12) << m_estimate;
  Ptr<Packet> p = Create<Packet> ((uint8_t*) msg.str().c_str(), msg.str().length());

  // Connect to the remote and send the packet
  m_socket_send->Connect (InetSocketAddress(Ipv4Address::ConvertFrom(dest), m_port));
  m_socket_send->Send(p);

  ++m_sent;
  // If we haven't hit the maximum number of packets to be sent, then schedule another one
  if (m_sent < m_count)
    {
      ScheduleTransmit (m_interval);
    }

  NS_LOG_INFO(
      Simulator::Now ().GetSeconds () // Time
      << " " << Ipv4Address::ConvertFrom(m_own_address) // own address
      << " " << "SEND" // initiating gossip 
      << " " << Ipv4Address::ConvertFrom (dest) // recipient
      << " " << m_sent // Number of packets sent
      << " " << std::setprecision(12) << m_estimate // current estimate
      );
}


void
BasicGossipServer::HandleReadWithoutResponse (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)))
    {
      // Extract a double from the received packet payload
      double dataAsDouble = RetrievePayload(packet);
      // Store old estimate
      m_old_estimate = m_estimate;
      // Update new estimate
      m_estimate = (m_estimate + dataAsDouble) / 2;

      NS_LOG_INFO(
          Simulator::Now ().GetSeconds () // Time
          << " " << Ipv4Address::ConvertFrom(m_own_address) // own address
          << " " << "RNOR" // receiving without responding
          << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () // sender
          << " " << m_old_estimate // previous estimate
          << " " << dataAsDouble // new information
          << " " << m_estimate //updated estimate
          << " " << fabs(m_estimate - m_old_estimate) // change
          );
    }
}


void 
BasicGossipServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)))
    {
      // Extract a double from the received packet payload
      double dataAsDouble = RetrievePayload(packet);
      // Store the old estimate
      m_old_estimate = m_estimate; 
      // Update the estimated mean
      m_estimate = (m_estimate + dataAsDouble) / 2;

      NS_LOG_INFO(
          Simulator::Now ().GetSeconds () // Time
          << " " << Ipv4Address::ConvertFrom(m_own_address) // own address
          << " " << "RECV" // receiving data from
          << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () // sender
          << " " << std::setprecision(12) << m_old_estimate // previous estimate
          << " " << std::setprecision(12) << dataAsDouble // new information
          << " " << m_estimate // updated estimate
          << " " << fabs(m_estimate - m_old_estimate) // change
          );

      // Put the updated mean into a packet and return it
      Ptr<Packet> p = MakePacket(m_estimate);
      socket->SendTo (p, 0, from);

      NS_LOG_INFO(
          Simulator::Now ().GetSeconds () // Time
          << " " << Ipv4Address::ConvertFrom(m_own_address) // own address
          << " " << "RESP" // responding to initiated gossip
          << " " << InetSocketAddress::ConvertFrom (from).GetIpv4 () // recipient of packet
          << " " << m_sent // number of packets this node has initiated
          << " " << std::setprecision(12) << m_estimate // updated estimate
          );
    }
}


Ptr<Packet>
BasicGossipServer::MakePacket (double estimate)
{
  std::ostringstream msg;
  msg << std::setprecision(12) << estimate;
  Ptr<Packet> p = Create<Packet> ((uint8_t*) msg.str().c_str(), msg.str().length());
  return p;
}


double
BasicGossipServer::RetrievePayload (Ptr<Packet> packet)
{
  uint8_t *buffer = new uint8_t[packet->GetSize()];
  packet->CopyData(buffer, packet->GetSize());
  std::string s = std::string((char*) buffer);
  return atof((char*) buffer);
}

} // Namespace ns3
