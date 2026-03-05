//
// Created by Belal Shehab on 28/02/2026.
//

#include <iostream>
#include <cstring>
#include <memory>
#include <mutex>
#include <sodium.h>

#include "GossipNode.h"
#include "nim_gossip_node.h"

#include <thread>

extern "C" void NimMain();

constexpr auto PEER_PRIVATE_KEY   = "b97cd5bf35f02cd8f5f916a4221ca44bc10034342147c16865a54d4807f57c22";
constexpr auto KEY_GUARD_PUBLIC_KEY = "cde70defd63dde1212450dd3aa92b2d2842bf317561d6d516c2b4031e342ff9a";

struct MockBlock {
    const uint8_t* data;
    size_t dataSize;
    uint8_t signature[crypto_sign_BYTES];
    size_t signatureSize = crypto_sign_BYTES;
};

MockBlock buildMockBlock(bool valid) {
    MockBlock block;
    const char* message = "Hello Block";
    block.data = reinterpret_cast<const uint8_t*>(message);
    block.dataSize = std::strlen(message);

    uint8_t seed[crypto_sign_SEEDBYTES];
    sodium_hex2bin(seed, sizeof(seed), PEER_PRIVATE_KEY, 64, nullptr, nullptr, nullptr);

    uint8_t peerPublicKey[crypto_sign_PUBLICKEYBYTES];
    uint8_t peerExpandedPrivateKey[crypto_sign_SECRETKEYBYTES];
    crypto_sign_seed_keypair(peerPublicKey, peerExpandedPrivateKey, seed);
    crypto_sign_detached(block.signature, nullptr, block.data, block.dataSize, peerExpandedPrivateKey);

    if (!valid) block.signature[0] = ~block.signature[0];
    return block;
}

// ── Cross-thread gossip signal ────────────────────────────────────────────────
struct GossipSignal {
    std::mutex mutex;
    MockBlock block;
};

static GossipSignal g_gossipSignal;
static KeyGuard::Client* g_keyGuardCap = nullptr;
static kj::Own<kj::CrossThreadPromiseFulfiller<void>>* g_fulfiller = nullptr;

// Called by Nim thread — must NOT touch KJ event loop directly
extern "C" int nimGossipNodeOnGossip(const uint8_t* data, size_t dataSize,
                                      const uint8_t* signature, size_t signatureSize) {
    std::cout << "[C++] onGossip called — signaling KJ event loop\n";
    if (!g_fulfiller || !g_keyGuardCap) {
        std::cerr << "[C++] onGossip not ready\n";
        return -1;
    }

    {
        static bool validBlock = false;
        validBlock = !validBlock;
        std::lock_guard<std::mutex> lock(g_gossipSignal.mutex);
        g_gossipSignal.block = buildMockBlock(validBlock);
    }

    (*g_fulfiller)->fulfill();
    return 0;
}



GossipNodeImpl::GossipNodeImpl(kj::StringPtr name)
    : m_name(name)
    , m_orchestrator(capnp::Capability::Client(nullptr).castAs<Orchestrator>())
{
}

kj::Promise<void> GossipNodeImpl::getName(GetNameContext context) {
    context.getResults().setName(m_name);
    return kj::READY_NOW;
}

kj::Promise<void> GossipNodeImpl::ping(PingContext context) {
    std::cout << "[GossipNode] Ping" << std::endl;
    return kj::READY_NOW;
}

kj::Promise<void> GossipNodeImpl::startListening(StartListeningContext context) {
    auto port = context.getParams().getPort();
    std::cout << "[GossipNode] Starting to listen on port " << port << "\n";

    return m_orchestrator.connectToKeyGuardRequest().send()
        .then([this, port](auto response) -> kj::Promise<void> {
            m_keyGuard    = kj::heap<KeyGuard::Client>(response.getKeyGuard());
            g_keyGuardCap = m_keyGuard.get();

            std::thread([port]() {
                NimMain();
                nimGossipNodeInit(static_cast<uint16_t>(port));
            }).detach();

            return gossipLoop();
        });
}

kj::Promise<void> GossipNodeImpl::gossipLoop() {
    auto paf = kj::newPromiseAndCrossThreadFulfiller<void>();
    m_fulfiller = kj::mv(paf.fulfiller);
    g_fulfiller = &m_fulfiller;

    return paf.promise.then([this]() -> kj::Promise<void> {
        std::cout << "[C++] gossipLoop — got gossip signal, forwarding to KeyGuard\n";

        MockBlock block;
        {
            std::lock_guard<std::mutex> lock(g_gossipSignal.mutex);
            block = g_gossipSignal.block;
        }

        auto req = g_keyGuardCap->validateBlockRequest();
        auto msg = req.initMessage();
        msg.setData(capnp::Data::Reader(block.data, block.dataSize));
        msg.setSignature(capnp::Data::Reader(block.signature, block.signatureSize));
        // senderId left empty for now — will be populated with real peer ID when libp2p is integrated

        return req.send().then([this](auto response) -> kj::Promise<void> {
            bool isValid = response.getIsValid();
            auto sigReader = response.getValidatorSignature();
            std::cout << "[C++] validation response: isValid=" << isValid << "\n";
            nimGossipNodeOnValidated(
                isValid,
                const_cast<uint8_t*>(sigReader.begin()),
                sigReader.size()
            );

            return gossipLoop();
        });
    });
}
