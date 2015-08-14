#ifndef VARCLUST_NODE_H
#define VARCLUST_NODE_H

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
   * \defgroup varclust
   */

  /**
   * \ingroup varclust
   * \brief A VarClust node
   *
   * Aggregates, etc.
   */
  class VarclustNode : public Application
  {
    public:

      /**
       * \brief Get the type ID.
       * \return the object TypeId
       */
      static TypeId GetTypeId (void);
      VarclustNode ();
      virtual ~VarclustNode ();

      /**
       * \brief set the list of addresses to which this node can send
       */
      void SetNeighbours (Ipv4InterfaceContainer interfaces);

      /**
       * \brief set this node's address (so that we can refer to it more easily)
       */
      void SetOwnAddress (Ipv4Address address);

    protected:
      virtual void DoDispose (void);

    private:
      virtual void StartApplication (void);
      virtual void StopApplication (void);

      /**
       * \brief Schedule the next packet transmission
       * \param dt time interval before next transmission
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
       * \brief Wrap values into a delimited string inside a packet
       */
      Ptr<Packet> MakePacket (double initial_measurement, double estimate_w, double estimate_w2);

      /**
       * \brief Extract double values from a packet payload
       */
      boost::tuple<double, double, double> ExtractPayload(Ptr<Packet> packet);

      /**
       * \brief Update the stored estimates
       */
      void Update (boost::tuple<double, double, double> data, Ipv4Address from);

      uint16_t m_port; //<! Port on which we listen for incoming packets
      Ptr<Socket> m_socket_passive; //<! Ipv4 Socket for the passive thread
      Ptr<Socket> m_socket_active; //<! Ipv4 Socket for the active thread
      Address m_local; //<! Local multicast address

      double m_initial_delay; //<! Initial delay before sending first packet
      uint32_t m_count; //<! Maximum number of packets the application will send
      uint32_t m_sent; //<! Counter for sent packets
      Time m_interval; //<! Interval between sending on the active thread

      double m_epsilon; //<! Epsilon value for terminating gossip

      double m_initial_measurement; //<! Initial measurement

      double m_estimate_w; //<! Estimate of the global mean
      double m_estimate_w2; //<! Estimate of the square of the global mean

      double m_old_estimate_w; //<! Previous value of m_estimate_w
      double m_old_estimate_w2; //<! Previous value of m_estimate_w2

      double m_variance; //<! Estimate of variance

      Ipv4InterfaceContainer m_interfaces; //<! Neighbours' addresses
      Ipv4Address m_own_address; //<! This node's address
      std::map<Ipv4Address, double> m_neighbour_measurements; // Neighbours' measurements
      std::map<Ipv4Address, bool> m_connectivity_map; //<! Connectivity decision map

      EventId m_sendEvent; //<! Event to send the next packet
  };
}

#endif /* VARCLUST_NODE_H */
