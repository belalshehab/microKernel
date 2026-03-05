//
// Created by Belal Shehab on 28/02/2026.
//

#pragma once

#include <capnp/ez-rpc.h>
#include <sodium.h>

#include "orchestrator.capnp.h"

class KeyGuardImpl final: public KeyGuard::Server {
public:
    explicit KeyGuardImpl(kj::StringPtr name);

    // Called by main() after rpc is constructed and bootstrap() is available
    void setOrchestrator(Orchestrator::Client orchestrator) {
        m_orchestrator = kj::mv(orchestrator);
    }

    kj::Promise<void> getName(GetNameContext context) override;
    kj::Promise<void> ping(PingContext context) override;
    kj::Promise<void> validateBlock(ValidateBlockContext context) override;

private:
    std::string m_name;
    Orchestrator::Client m_orchestrator;
    uint8_t m_keyGuardExpandedPrivateKey[crypto_sign_SECRETKEYBYTES];
    uint8_t m_keyGuardPublicKey[crypto_sign_PUBLICKEYBYTES];
    uint8_t m_peerPublicKey[crypto_sign_PUBLICKEYBYTES];
};