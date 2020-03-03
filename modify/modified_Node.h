/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/node.h"

namespace ns3 {

    class modified_Node : public Node
    {
    private:
        /* data */
        uint32_t condition;
        
    public:
        modified_Node(uint32_t c = 0)
        {
            condition = c;
        };
        ~modified_Node();

        uint32_t getCondition();
    };
    
}