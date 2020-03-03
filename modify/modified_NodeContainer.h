/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/node-container.h"

namespace ns3{

    class modified_NodeContainer : public NodeContainer
    {
    private:
        /* data */
        std::vector<Ptr<modified_Node> > m_nodes; //!< Nodes smart pointers
    public:
        modified_NodeContainer(/* args */);
        ~modified_NodeContainer();
        void Create (uint32_t n);
    };
    
}