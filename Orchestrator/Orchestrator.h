//
// Created by Belal Shehab on 28/02/2026.
//

#pragma once

#include <capnp/ez-rpc.h>
#include "orchestrator.capnp.h"


class OrchestratorImpl: public Orchestrator::Server {
    public:
    // Explicitly defined default constructor — initializes clients to null caps
    OrchestratorImpl();
    OrchestratorImpl(KeyGuard::Client keyGuard, GossipNode::Client gossipNode);

    // Setters — called after both services are connected
    void setKeyGuard(KeyGuard::Client keyGuard)         { m_keyGuard    = kj::mv(keyGuard);    }
    void setGossipNode(GossipNode::Client gossipNode)   { m_gossipNode  = kj::mv(gossipNode);  }

    kj::Promise<void> getServices(GetServicesContext context) override;
    kj::Promise<void> connectToKeyGuard(ConnectToKeyGuardContext context) override;
    kj::Promise<void> connectToGossipNode(ConnectToGossipNodeContext context) override;

private:
    KeyGuard::Client    m_keyGuard;
    GossipNode::Client  m_gossipNode;
};
