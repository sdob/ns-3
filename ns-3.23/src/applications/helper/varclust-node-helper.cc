#include "varclust-node-helper.h"
#include "ns3/varclust-node.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {
  
  VarclustNodeHelper::VarclustNodeHelper (uint16_t port) {
    m_factory.SetTypeId (VarclustNode::GetTypeId ());
    SetAttribute ("Port", UintegerValue (port));
  }


  void VarclustNodeHelper::SetAttribute (std::string name, const AttributeValue &value) {
    m_factory.Set (name, value);
  }


  ApplicationContainer VarclustNodeHelper::Install (Ptr<Node> node) const {
    return ApplicationContainer (InstallPriv (node));
  }


  ApplicationContainer VarclustNodeHelper::Install (std::string nodeName) const {
    Ptr<Node> node = Names::Find<Node> (nodeName);
    return ApplicationContainer (InstallPriv (node));
  }


  ApplicationContainer VarclustNodeHelper::Install (NodeContainer c) const {
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
      apps.Add (InstallPriv (*i));
    }
    return apps;
  }


  Ptr<Application> VarclustNodeHelper::InstallPriv (Ptr<Node> node) const {
    Ptr<Application> app = m_factory.Create<VarclustNode> ();
    node->AddApplication (app);

    return app;
  }
}
