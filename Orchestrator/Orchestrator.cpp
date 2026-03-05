//
// Created by Belal Shehab on 28/02/2026.
//

#include "Orchestrator.h"


// Default constructor: initializes both clients to null caps.
// Clients are injected later via setValidator() / setListener().
OrchestratorImpl::OrchestratorImpl()
    : m_keyGuard(capnp::Capability::Client(nullptr).castAs<KeyGuard>())
    , m_gossipNode(capnp::Capability::Client(nullptr).castAs<GossipNode>())
{
}

OrchestratorImpl::OrchestratorImpl(KeyGuard::Client keyGuard, GossipNode::Client gossipNode) :
    m_keyGuard(kj::mv(keyGuard))
    , m_gossipNode(kj::mv(gossipNode))
{
}

kj::Promise<void> OrchestratorImpl::connectToKeyGuard(ConnectToKeyGuardContext context) {
    context.getResults().setKeyGuard(m_keyGuard);
    return kj::READY_NOW;
}

kj::Promise<void> OrchestratorImpl::connectToGossipNode(ConnectToGossipNodeContext context) {
    context.getResults().setGossipNode(m_gossipNode);
    return kj::READY_NOW;
}

kj::Promise<void> OrchestratorImpl::getServices(GetServicesContext context) {
    auto results = context.getResults();
    auto list = results.initServices(2);
    list.set(0, "keyGuard");
    list.set(1, "gossipNode");
    return kj::READY_NOW;
}
