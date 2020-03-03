/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "modified_NodeContainer.h"
#include "modified_Node.h"

namespace ns3{

  void
  modified_NodeContainer::Create (uint32_t n)
  {
    for (uint32_t i = 0; i < n; i++)
      {
        m_nodes.push_back (CreateObject<modified_Node> ());
      }
  }

}