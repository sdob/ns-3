#ifndef MULTIPHASE_VARCLUST_NODE_H
#define MULTIPHASE_VARCLUST_NODE_H

#include <map>

#include "boost/tuple/tuple.hpp"

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
   * \defgroup multiphase-varclust
   */

  /**
   * \ingroup multiphase-varclust
   * \brief A multiphase VarClust node
   */
  class MultiphaseVarclustNode : public Application {
    public:
      /**
       * \brief Get the type ID.
       */
      static TypeId GetTypeId (void);
      MultiphaseVarclustNode ();
      virtual ~MultiphaseVarclustNode ();

      /**
       * \brief set the list of addresses that this node knows about
       */
      void SetNeighbours (Ipv4InterfaceContainer interfaces);

      /**
       * \brief set this node's address
       */
      void SetOwnAddress (Ipv4Address address);

    protected:
      virtual void DoDispose (void);

    private:
      virtual void StartApplication (void);
      virtual void StopApplication (void);

      /**
       * \brief Get a random neighbour
       */
      Ipv4Address GetNeighbour (void);

      /**
       * \brief Schedule the next packet transmission
       */
      void ScheduleTransmit (Time dt);

      /**
       * \brief Send a packet
       */
      void Send (void);

      /**
       * \brief Handle packet reception on the passive thread
       */
      void HandlePassiveRead (Ptr<Socket> socket);

      /**
       * \brief Handle packet reception on the active thread
       */
      void HandleActiveRead (Ptr<Socket> socket);

      /**
       * \brief wrap values into a delimited string inside a packet
       */
      Ptr<Packet> MakePacket (int epoch, double initial_measurement, double estimate_w, double estimate_w2);

      /**
       * \brief Begin a new epoch
       */
      void StartEpoch (bool changed_this_epoch);

      /**
       * \brief Extract epoch, measurement, and estimates from a packet payload
       */
      boost::tuple<int, double, double, double> ExtractPayload (Ptr<Packet> packet);

      /**
       * \brief Update the stored estimates
       */
      void Update(boost::tuple<int, double, double, double> data, Ipv4Address from);

      /**
       * \brief Update the connectivity decision map
       */
      void UpdateConnectivityDecisions (void);

      /**
       * \brief Update the stored epoch
       */
      void UpdateEpoch (int epoch);

      uint16_t m_port; //<! Port on which we listen for incoming packets
      Ptr<Socket> m_socket_passive; //<! Ipv4 Socket for the passive thread
      Ptr<Socket> m_socket_active; //<! Ipv4 Socket for the active thread
      Address m_local; //<! Local multicast address

      double m_initial_delay; //<! Initial delay before sending first packet
      Time m_interval; //<! Interval between sending on the active thread

      int m_length_of_epoch; //<! Length (in terms of messages sent) of each epoch
      int m_messages_this_epoch; //<! Number of messages sent this epoch
      int m_current_epoch; //<! Current phase of VarClust
      bool m_changed_this_epoch; //<! Has connectivity changed during the current epoch?

      uint32_t m_count; //<! Maximum number of packets the application will send
      uint32_t m_sent;//<! Counter for sent packets

      double m_epsilon; //<! Epsilon for terminating gossip
      double m_initial_measurement; //<! Initial measurement
      double m_estimate_w; //<! Estimate of the global mean
      double m_estimate_w2; //<! Estimate of the square of the global mean
      double m_old_estimate_w; //<! Previous estimate of the global mean
      double m_old_estimate_w2; //<! Previous estimate of the square of the global mean
      double m_variance; //<! Estimate of global variance

      Ipv4InterfaceContainer m_interfaces; //<! Neighbours' addresses
      Ipv4Address m_own_address; //<! This node's address
      std::map<Ipv4Address, double> m_neighbour_measurements; //<! Neighbours' initial measurements

      std::map<Ipv4Address, bool> m_connectivity_map; //<! Connectivity map for the current epoch
      std::map<Ipv4Address, bool> m_connectivity_decisions; //<! Connectivity decision map

      EventId m_sendEvent; //<! Event to send the next packet
  };
}

#endif
