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

#ifndef BASIC_GOSSIP_SERVER_H
#define BASIC_GOSSIP_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/ipv4-interface-container.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications 
 * \defgroup basicgossip BasicGossip
 */

/**
 * \ingroup basicgossip
 * \brief A Basic Gossip server
 *
 * Every packet received is sent back.
 */
class BasicGossipServer : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  BasicGossipServer ();
  virtual ~BasicGossipServer ();

  /**
   * \brief set the list of addresses to which this node can send
   */
  void SetNeighbours (Ipv4InterfaceContainer interfaces);

  /**
   * \brief set this server's address so that we can read it easily
   */
  void SetOwnAddress (Address address);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Schedule the next packet transmission
   * \param dt time interval between packets.
   */
  void ScheduleTransmit (Time dt);

  /**
   * \brief Send a packet
   */
  void Send (void);

  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void HandleRead (Ptr<Socket> socket);

  /**
   * \brief Handle a packet reception without responding.
   */
  void HandleReadWithoutResponse (Ptr<Socket> socket);

  /**
   * \brief Create a packet containing a double value.
   */
  Ptr<Packet> MakePacket (double estimate);

  /**
   * \brief Extract a double value from a packet.
   */
  double RetrievePayload (Ptr<Packet> packet);


  uint16_t m_port; //!< Port on which we listen for incoming packets.
  Ptr<Socket> m_socket; //!< IPv4 Socket for receiving
  Ptr<Socket> m_socket_send; //!< IPv4 Socket for sending
  Address m_local; //!< local multicast address

  uint32_t m_count; //!< Maximum number of packets the application will send
  uint32_t m_sent; //!< Counter for sent packets
  Time m_interval; //!< Packet inter-send time

  double m_estimate; //!< The estimate being held by the node
  double m_old_estimate; //!< The previous estimate
  double m_epsilon; //!< Convergence epsilon for gossip

  Ipv4InterfaceContainer m_interfaces; //!< Neighbours' addresses
  Address m_own_address; //!< Server's own address

  EventId m_sendEvent; //!< Event to send the next packet

};

} // namespace ns3

#endif /* BASIC_GOSSIP_SERVER_H */

