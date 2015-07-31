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
#include "basic-gossip-helper.h"
#include "ns3/basic-gossip-server.h"
#include "ns3/basic-gossip-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

BasicGossipServerHelper::BasicGossipServerHelper (uint16_t port)
{
  m_factory.SetTypeId (BasicGossipServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
BasicGossipServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
BasicGossipServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BasicGossipServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BasicGossipServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
BasicGossipServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<BasicGossipServer> ();
  node->AddApplication (app);

  return app;
}

BasicGossipClientHelper::BasicGossipClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (BasicGossipClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

BasicGossipClientHelper::BasicGossipClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (BasicGossipClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

BasicGossipClientHelper::BasicGossipClientHelper (Ipv6Address address, uint16_t port)
{
  m_factory.SetTypeId (BasicGossipClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

void 
BasicGossipClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
BasicGossipClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<BasicGossipClient>()->SetFill (fill);
}

void
BasicGossipClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<BasicGossipClient>()->SetFill (fill, dataLength);
}

void
BasicGossipClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<BasicGossipClient>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
BasicGossipClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BasicGossipClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
BasicGossipClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
BasicGossipClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<BasicGossipClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
