//
// Created by Belal Shehab on 28/02/2026.
//

#pragma once

#include <capnp/ez-rpc.h>
#include "orchestrator.capnp.h"
#include <string>
#include <mutex>
#include <vector>

class GossipNodeImpl final: public GossipNode::Server {

public:
    explicit GossipNodeImpl(kj::StringPtr name);

    void setOrchestrator(Orchestrator::Client orchestrator) {
        m_orchestrator = kj::mv(orchestrator);
    }

    kj::Promise<void> getName(GetNameContext context) override;
    kj::Promise<void> ping(PingContext context) override;
    kj::Promise<void> startListening(StartListeningContext context) override;
    kj::Promise<void> publishData(PublishDataContext context) override;

private:
    kj::Promise<void> gossipLoop();

    std::string          m_name;
    Orchestrator::Client m_orchestrator;
    kj::Own<KeyGuard::Client> m_keyGuard;
    kj::Own<kj::CrossThreadPromiseFulfiller<void>> m_fulfiller;
};
