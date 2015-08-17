#include "multiphase-varclust-node-helper.h"
#include "ns3/multiphase-varclust-node.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {
  MultiphaseVarclustNodeHelper::MultiphaseVarclustNodeHelper (uint16_t port) {
    m_factory.SetTypeId (MultiphaseVarclustNode::GetTypeId ());
    SetAttribute ("Port", UintegerValue (port));
  }

  void MultiphaseVarclustNodeHelper::SetAttribute (std::string name, const AttributeValue &value) {
    m_factory.Set (name, value);
  }

  ApplicationContainer MultiphaseVarclustNodeHelper::Install (Ptr <Node> node) const {
    return ApplicationContainer (InstallPriv (node));
  }

  ApplicationContainer MultiphaseVarclustNodeHelper::Install (std::string nodeName) const {
    Ptr<Node> node = Names::Find<Node> (nodeName);
    return ApplicationContainer (InstallPriv (node));
  }

  ApplicationContainer MultiphaseVarclustNodeHelper::Install (NodeContainer c) const {
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
      apps.Add (InstallPriv (*i));
    }
    return apps;
  }

  Ptr<Application> MultiphaseVarclustNodeHelper::InstallPriv (Ptr<Node> node) const {
    Ptr<Application> app = m_factory.Create<MultiphaseVarclustNode> ();
    node->AddApplication (app);
    return app;
  }
}
