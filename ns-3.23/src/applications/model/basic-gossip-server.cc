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
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&BasicGossipServer::SetDataSize,
                                         &BasicGossipServer::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
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
    .AddAttribute("Epsilon", "The minimum change in estimate for a node to continue gossip",
                  DoubleValue (pow(10, -6)),
                  MakeDoubleAccessor (&BasicGossipServer::m_epsilon),
                  MakeDoubleChecker<double> ())
  ;
  return tid;
}

BasicGossipServer::BasicGossipServer ()
{
  NS_LOG_FUNCTION (this);
  // Initialize m_data and m_dataSize
  m_data = 0;
  m_dataSize = 0;
  m_sendEvent = EventId ();
  m_sent = 0; // number of packets sent
}

BasicGossipServer::~BasicGossipServer()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socket6 = 0;
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
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
  // TODO: implement this
  m_sendEvent = Simulator::Schedule (dt, &BasicGossipServer::Send, this);
}

void
BasicGossipServer::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  // Choose a random element of the interfaces vector --- assumes that there
  // are at least two interfaces
  // TODO: introduce some error checking in case somebody does something stupid
  // and tries to run the experiment with only one node
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue(0));
  x->SetAttribute ("Max", DoubleValue(m_interfaces.GetN() - 1)); // range is [min, max) exclusive for doubles 
  int index = x->GetInteger ();
  while (m_interfaces.GetAddress(index) == m_own_address) {
    index = x->GetInteger ();
  }
  Address dest = m_interfaces.GetAddress(index);

  // Now we've got an address to send to, let's build a packet
  Ptr<Packet> p;
  double msg = m_estimate;
  m_dataSize = sizeof(msg);
  std::stringstream s;
  s << msg;
  std::string fill = s.str ();
  uint32_t dataSize = fill.size() + 1;
  if (dataSize != m_dataSize) {
    delete [] m_data;
    m_data = new uint8_t [dataSize];
    m_dataSize = dataSize;
  }
  memcpy(m_data, s.str().c_str(), m_dataSize);
  p = Create<Packet> (m_data, m_dataSize);

  // Create a socket and fire off the packet
  // TODO: I don't think this a great idea, since port numbers seem to keep going up
  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  m_socket_send = Socket::CreateSocket (GetNode (), tid);
  m_socket_send->Bind();
  m_socket_send->Connect (InetSocketAddress(Ipv4Address::ConvertFrom(dest), m_port));
  m_socket_send->Send(p);

  NS_LOG_INFO("At time " << Simulator::Now ().GetSeconds () << "s node at " << Ipv4Address::ConvertFrom(m_own_address) << " sent to " <<
      Ipv4Address::ConvertFrom (dest) << " port " << m_port);

  ++m_sent;
  // If we haven't hit the maximum number of packets to be sent, then schedule another one
  if (m_sent < m_count)
    {
      ScheduleTransmit (m_interval);
    }
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
  m_own_address = address;
  NS_LOG_FUNCTION(this << m_own_address);
}

void 
BasicGossipServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      //Ipv4Address any = Ipv4Address::GetAny ();
      //NS_LOG_INFO("got address: " << any);
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

  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::GetAny (), m_port);
      m_socket6->Bind (local6);
      if (addressUtils::IsMulticast (local6))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket6);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, local6);
            }
          else
            {
              NS_FATAL_ERROR ("Error: Failed to join multicast group");
            }
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&BasicGossipServer::HandleRead, this));
  m_socket6->SetRecvCallback (MakeCallback (&BasicGossipServer::HandleRead, this));

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
  if (m_socket6 != 0) 
    {
      m_socket6->Close ();
      m_socket6->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  Simulator::Cancel (m_sendEvent);
}

void 
BasicGossipServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes from " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }

      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();

      //NS_LOG_FUNCTION (this << packet);
      //NS_LOG_INFO("Packet looks like: " << packet->ToString());

      // Send a packet reflecting the m_size attribute
      Ptr<Packet> p = Create<Packet> (m_size);
      socket->SendTo (p, 0, from);
      //NS_LOG_FUNCTION(this << result);


      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server sent " << packet->GetSize () << " bytes to " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server sent " << packet->GetSize () << " bytes to " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }
    }
}

void 
BasicGossipServer::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the server is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so 
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t 
BasicGossipServer::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

} // Namespace ns3
