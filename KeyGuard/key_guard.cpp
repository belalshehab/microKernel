#include <iostream>
#include "key_guard.h"

constexpr auto PEER_PUBLIC_KEY    = "45b89ba5700a7011e97e3159a5618fa55fb537e5188832df9d83cbb3bfe9eb4a";
constexpr auto KEY_GUARD_PRIVATE_KEY = "e03e8bda99fbdc02b9059c205aba18fc1c10f3d83abc414a9d158aa4282da73f";

KeyGuardImpl::KeyGuardImpl(kj::StringPtr name)
    : m_name(name)
    , m_orchestrator(capnp::Capability::Client(nullptr).castAs<Orchestrator>())
{
    sodium_hex2bin(
        m_peerPublicKey, sizeof(m_peerPublicKey),
        PEER_PUBLIC_KEY, 64,
        nullptr, nullptr, nullptr
    );

    uint8_t seed[crypto_sign_SEEDBYTES];
    sodium_hex2bin(
        seed, sizeof(seed),
        KEY_GUARD_PRIVATE_KEY, 64,
        nullptr, nullptr, nullptr
    );

    crypto_sign_seed_keypair(
        m_keyGuardPublicKey,
        m_keyGuardExpandedPrivateKey,
        seed
    );
}

kj::Promise<void> KeyGuardImpl::getName(GetNameContext context) {
    context.getResults().setName(m_name);
    return kj::READY_NOW;
}

kj::Promise<void> KeyGuardImpl::ping(PingContext context) {
    std::cout << "[KeyGuard] Ping" << std::endl;
    return kj::READY_NOW;
}

kj::Promise<void> KeyGuardImpl::validateBlock(ValidateBlockContext context) {
    auto msg       = context.getParams().getMessage();
    auto data      = msg.getData();
    auto signature = msg.getSignature();
    // senderId available as msg.getSenderId() for future per-peer key lookup
    std::cout << "[KeyGuard] Validating block, data size: " << data.size()
              << ", sig size: " << signature.size() << "\n";

    int result = crypto_sign_verify_detached(
        signature.begin(),
        data.begin(),
        data.size(),
        m_peerPublicKey
    );
    bool isValid = (result == 0);
    std::cout << "[KeyGuard] Block signature: " << (isValid ? "VALID" : "INVALID") << "\n";

    std::vector<uint8_t> message(data.begin(), data.end());
    message.push_back(isValid ? 0x01 : 0x00);

    uint8_t keyGuardSignature[crypto_sign_BYTES];
    crypto_sign_detached(
        keyGuardSignature,
        nullptr,
        message.data(),
        message.size(),
        m_keyGuardExpandedPrivateKey
    );
    std::cout << "[KeyGuard] Response signed with KeyGuard private key\n";

    context.getResults().setIsValid(isValid);
    context.getResults().setValidatorSignature(
        capnp::Data::Reader(keyGuardSignature, sizeof(keyGuardSignature))
    );
    return kj::READY_NOW;
}
