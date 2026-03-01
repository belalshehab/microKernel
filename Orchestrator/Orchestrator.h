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
    OrchestratorImpl(Validator::Client validator, NetworkListener::Client listener);

    // Setters — called after both services are connected
    void setValidator(Validator::Client validator)         { m_validator = kj::mv(validator); }
    void setListener(NetworkListener::Client listener)     { m_listener  = kj::mv(listener);  }

    kj::Promise<void> getServices(GetServicesContext context) override;
    kj::Promise<void> connectToValidator(ConnectToValidatorContext context) override;
    kj::Promise<void> connectToNetworkListener(ConnectToNetworkListenerContext context) override;

private:
    Validator::Client       m_validator;
    NetworkListener::Client m_listener;
};
