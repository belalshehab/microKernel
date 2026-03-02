#include <iostream>
#include "validator.h"

constexpr auto LISTENER_PUBLIC_KEY = "45b89ba5700a7011e97e3159a5618fa55fb537e5188832df9d83cbb3bfe9eb4a";
constexpr auto VALIDATOR_PRIVATE_KEY = "e03e8bda99fbdc02b9059c205aba18fc1c10f3d83abc414a9d158aa4282da73f";

ValidatorImpl::ValidatorImpl(kj::StringPtr name)
    : m_name(name)
    , m_orchestrator(capnp::Capability::Client(nullptr).castAs<Orchestrator>())
{
    sodium_hex2bin(
        m_listenerPublicKey, sizeof(m_listenerPublicKey),
        LISTENER_PUBLIC_KEY, 64,
        nullptr,
        nullptr,
        nullptr
    );

    uint8_t seed[crypto_sign_SEEDBYTES];
    sodium_hex2bin(
        seed, sizeof(seed),
        VALIDATOR_PRIVATE_KEY, 64,
        nullptr,
        nullptr,
        nullptr
    );

    crypto_sign_seed_keypair(
        m_validatorPublicKey,
        m_validatorExpandedPrivateKey,
        seed
    );

}

kj::Promise<void> ValidatorImpl::getName(GetNameContext context) {
    context.getResults().setName(m_name);
    return kj::READY_NOW;
}

kj::Promise<void> ValidatorImpl::ping(PingContext context) {
    std::cout << "[Validator] Ping" << std::endl;
    return kj::READY_NOW;
}

kj::Promise<void> ValidatorImpl::validateBlock(ValidateBlockContext context) {
    auto data = context.getParams().getData();
    auto signature = context.getParams().getSignature();
    std::cout << "[Validator] Validating block, data size: " << data.size()
              << ", sig size: " << signature.size() << "\n";


    // verify the signature using libsodium
    int result = crypto_sign_verify_detached(
        signature.begin(),
        data.begin(),
        data.size(),
        m_listenerPublicKey
    );
    bool isValid = (result == 0);
    std::cout << "[Validator] Block signature: " << (isValid ? "VALID" : "INVALID") << "\n";

    // append isValid byte to the message so the listener can verify exactly what we signed
    std::vector<uint8_t> message(data.begin(), data.end());
    message.push_back(isValid ? 0x01 : 0x00);

    uint8_t validatorSignature[crypto_sign_BYTES];
    crypto_sign_detached(
        validatorSignature,
        nullptr,
        message.data(),
        message.size(),
        m_validatorExpandedPrivateKey
    );
    std::cout << "[Validator] Response signed with validator private key\n";

    context.getResults().setIsValid(isValid);
    context.getResults().setValidatorSignature(
        capnp::Data::Reader(validatorSignature, sizeof(validatorSignature))
    );
    return kj::READY_NOW;
}
