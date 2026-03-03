//
// Created by Belal Shehab on 28/02/2026.
//

#include <iostream>
#include <cstring>
#include <memory>
#include <mutex>
#include <sodium.h>

#include "NetworkListener.h"
#include "nim_listener.h"

#include <thread>

extern "C" void NimMain();

constexpr auto LISTENER_PRIVATE_KEY = "b97cd5bf35f02cd8f5f916a4221ca44bc10034342147c16865a54d4807f57c22";
constexpr auto VALIDATOR_PUBLIC_KEY = "cde70defd63dde1212450dd3aa92b2d2842bf317561d6d516c2b4031e342ff9a";

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
    sodium_hex2bin(seed, sizeof(seed), LISTENER_PRIVATE_KEY, 64, nullptr, nullptr, nullptr);

    uint8_t listenerPublicKey[crypto_sign_PUBLICKEYBYTES];
    uint8_t listenerExpandedPrivateKey[crypto_sign_SECRETKEYBYTES];
    crypto_sign_seed_keypair(listenerPublicKey, listenerExpandedPrivateKey, seed);
    crypto_sign_detached(block.signature, nullptr, block.data, block.dataSize, listenerExpandedPrivateKey);

    if (!valid) block.signature[0] = ~block.signature[0];
    return block;
}

// ── Cross-thread gossip signal ────────────────────────────────────────────────
// Nim thread writes gossip data here and fulfills the promise.
// KJ thread reads from here inside the .then() callback.
struct GossipSignal {
    std::mutex mutex;
    MockBlock block;  // pre-built block, ready to send to validator
};

static GossipSignal g_gossipSignal;
static Validator::Client* g_validatorCap = nullptr;
static kj::Own<kj::CrossThreadPromiseFulfiller<void>>* g_fulfiller = nullptr;

// Called by Nim thread — must NOT touch KJ event loop directly
extern "C" int nimListenerOnGossip(const uint8_t* data, size_t dataSize,
                                    const uint8_t* signature, size_t signatureSize) {
    std::cout << "[C++] onGossip called — signaling KJ event loop\n";
    if (!g_fulfiller || !g_validatorCap) {
        std::cerr << "[C++] onGossip not ready\n";
        return -1;
    }

    // 1. Build mock block into shared struct (thread-safe via mutex)
    {
        static bool validBlock = false;
        validBlock = validBlock ? false : true;  // alternate valid/invalid blocks for testing
        std::lock_guard<std::mutex> lock(g_gossipSignal.mutex);
        g_gossipSignal.block = buildMockBlock(validBlock);  // alternate valid/invalid blocks for testing
    }

    // 2. Signal the KJ thread — this is the only thread-safe KJ call
    (*g_fulfiller)->fulfill();
    return 0;
}



NetworkListenerImpl::NetworkListenerImpl(kj::StringPtr name)
    : m_name(name)
    , m_orchestrator(capnp::Capability::Client(nullptr).castAs<Orchestrator>())
{
}

kj::Promise<void> NetworkListenerImpl::getName(GetNameContext context) {
    context.getResults().setName(m_name);
    return kj::READY_NOW;
}

kj::Promise<void> NetworkListenerImpl::ping(PingContext context) {
    std::cout << "[NetworkListener] Ping" << std::endl;
    return kj::READY_NOW;
}

kj::Promise<void> NetworkListenerImpl::startListening(StartListeningContext context) {
    auto port = context.getParams().getPort();
    std::cout << "[NetworkListener] Starting to listen on port " << port << "\n";

    return m_orchestrator.connectToValidatorRequest().send()
        .then([this, port](auto response) -> kj::Promise<void> {
            m_validator    = kj::heap<Validator::Client>(response.getValidator());
            g_validatorCap = m_validator.get();

            // Start Nim on a background thread — it will call nimListenerOnGossip
            // which is now just a mutex write + fulfill(). No KJ calls on Nim thread.
            std::thread([port]() {
                NimMain();
                nimListenerInit(static_cast<uint16_t>(port));
            }).detach();

            // Start the gossip event loop on the KJ thread
            return gossipLoop();
        });
}

// Runs entirely on the KJ event loop thread.
// Waits for Nim to signal via fulfiller, reads gossip data, calls validator, repeats.
kj::Promise<void> NetworkListenerImpl::gossipLoop() {
    // Create a new promise/fulfiller pair for this iteration
    auto paf = kj::newPromiseAndCrossThreadFulfiller<void>();
    m_fulfiller = kj::mv(paf.fulfiller);
    g_fulfiller = &m_fulfiller;

    // Wait for Nim to signal, then handle it and loop again
    return paf.promise.then([this]() -> kj::Promise<void> {
        std::cout << "[C++] gossipLoop — got gossip signal, forwarding to validator\n";

        // Read the pre-built block under mutex — constructed in onGossip on the Nim thread
        MockBlock validBlock;
        {
            std::lock_guard<std::mutex> lock(g_gossipSignal.mutex);
            validBlock = g_gossipSignal.block;
        }

        auto req = g_validatorCap->validateBlockRequest();
        req.setData(capnp::Data::Reader(validBlock.data, validBlock.dataSize));
        req.setSignature(capnp::Data::Reader(validBlock.signature, validBlock.signatureSize));

        return req.send().then([this](auto response) -> kj::Promise<void> {
            bool isValid = response.getIsValid();
            auto sigReader = response.getValidatorSignature();
            std::cout << "[C++] validation response: isValid=" << isValid << "\n";
            nimListenerOnValidated(
                isValid,
                const_cast<uint8_t*>(sigReader.begin()),
                sigReader.size()
            );
            // Re-arm — wait for next gossip signal
            return gossipLoop();
        });
    });
}
