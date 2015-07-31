/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "basic-random-gossip-helper.h"
#include "ns3/basic-random-gossip-server.h"
#include "ns3/basic-random-gossip-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

BasicRandomGossipServerHelper::BasicRandomGossipServerHelper (uint16_t port)
{
  m_factory.SetTypeId (BasicRandomGossipServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
BasicRandomGossipServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
BasicRandomGossipServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BasicRandomGossipServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BasicRandomGossipServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
BasicRandomGossipServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<BasicRandomGossipServer> ();
  node->AddApplication (app);

  return app;
}

BasicRandomGossipClientHelper::BasicRandomGossipClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (BasicRandomGossipClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

BasicRandomGossipClientHelper::BasicRandomGossipClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (BasicRandomGossipClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

BasicRandomGossipClientHelper::BasicRandomGossipClientHelper (Ipv6Address address, uint16_t port)
{
  m_factory.SetTypeId (BasicRandomGossipClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

void 
BasicRandomGossipClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

/*
void
BasicRandomGossipClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<BasicRandomGossipClient>()->SetFill (fill);
}
*/

void
BasicRandomGossipClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<BasicRandomGossipClient>()->SetFill (fill, dataLength);
}

/*
void
BasicRandomGossipClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<BasicRandomGossipClient>()->SetFill (fill, fillLength, dataLength);
}
*/

ApplicationContainer
BasicRandomGossipClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BasicRandomGossipClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BasicRandomGossipClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
BasicRandomGossipClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<BasicRandomGossipClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
