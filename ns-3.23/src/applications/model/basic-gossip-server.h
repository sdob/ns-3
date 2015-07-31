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

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void HandleRead (Ptr<Socket> socket);

  void SetDataSize (uint32_t dataSize);
  uint32_t GetDataSize (void) const;

  uint16_t m_port; //!< Port on which we listen for incoming packets.
  uint32_t m_size; //<! Size of the sent packet
  uint32_t m_dataSize; //!< Packet payload size (must be equal to m_size)
  uint8_t *m_data; //!< Packet data
  Ptr<Socket> m_socket; //!< IPv4 Socket
  Ptr<Socket> m_socket6; //!< IPv6 Socket
  Address m_local; //!< local multicast address
};

} // namespace ns3

#endif /* BASIC_GOSSIP_SERVER_H */

