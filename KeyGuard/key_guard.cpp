#include <iostream>
#include "key_guard.h"

constexpr auto KEY_GUARD_PRIVATE_KEY = "e03e8bda99fbdc02b9059c205aba18fc1c10f3d83abc414a9d158aa4282da73f";

static const std::vector<std::pair<std::string, const char*>> KNOWN_NODES = {
    { "node-1", "45b89ba5700a7011e97e3159a5618fa55fb537e5188832df9d83cbb3bfe9eb4a" }
};

KeyGuardImpl::KeyGuardImpl(kj::StringPtr name)
    : m_name(name)
    , m_orchestrator(capnp::Capability::Client(nullptr).castAs<Orchestrator>())
{
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

    for (auto & [id, hexPubKey] : KNOWN_NODES) {
        std::array<uint8_t, crypto_sign_PUBLICKEYBYTES> pkArr;
        sodium_hex2bin(pkArr.data(), pkArr.size(), hexPubKey, 64, nullptr, nullptr, nullptr);
        m_trustedPeers[id] = pkArr;
        std::cout << "[KeyGuard] Hardcoded known node: " << id << "\n";
    }
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
    auto senderId = msg.getSenderId();
    // senderId available as msg.getSenderId() for future per-peer key lookup
    std::cout << "[KeyGuard] Validating block, data size: " << data.size()
              << ", sig size: " << signature.size() << "\n";

    std::string key(reinterpret_cast<const char*>(senderId.begin()), senderId.size());

    auto it = m_trustedPeers.find(key);
    if (it == m_trustedPeers.end()) {
        std::cout << "[KeyGuard] Unknown sender — rejecting\n";
        context.getResults().setIsValid(false);
        context.getResults().setValidatorSignature(capnp::Data::Reader(nullptr, 0));
        return kj::READY_NOW;
    }

    int result = crypto_sign_verify_detached(
        signature.begin(),
        data.begin(),
        data.size(),
        it->second.data()
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

kj::Promise<void> KeyGuardImpl::signData(SignDataContext context) {
    auto data = context.getParams().getData();

    uint8_t signature[crypto_sign_BYTES];
    crypto_sign_detached(
        signature,
        nullptr,
        data.begin(),
        data.size(),
        m_keyGuardExpandedPrivateKey
    );

    std::cout << "[KeyGuard] Data signed with KeyGuard private key, data size: " << data.size() << "\n";
    context.getResults().setSignature(capnp::Data::Reader(signature, sizeof(signature)));
    return kj::READY_NOW;
}

kj::Promise<void> KeyGuardImpl::addTrustedPeer(AddTrustedPeerContext context) {
    auto peerId = context.getParams().getPeerId();
    auto publicKeyData = context.getParams().getPublicKey();

    if (publicKeyData.size() != crypto_sign_PUBLICKEYBYTES) {
        std::cerr << "[KeyGuard] Invalid public key size: " << publicKeyData.size() << "\n";
        return kj::READY_NOW;
    }

    std::string key(reinterpret_cast<const char*>(peerId.begin()), peerId.size());
    std::array<uint8_t, crypto_sign_PUBLICKEYBYTES> publicKeyArr;
    std::memcpy(publicKeyArr.data(), publicKeyData.begin(), crypto_sign_PUBLICKEYBYTES);
    m_trustedPeers[key] = publicKeyArr;
    std::cout << "[KeyGuard] Added trusted peer, id size: " << peerId.size() << "\n";
    return kj::READY_NOW;
}
