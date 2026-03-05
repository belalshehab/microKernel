//
// Created by Belal Shehab on 03/03/2026.
//

#pragma once
#include <cstdint>
#include <cstddef>


extern "C" {
  void nimGossipNodeInit(uint16_t port);

  int nimGossipNodeOnGossip(const uint8_t* data, size_t dataSize, const uint8_t* signature, size_t signatureSize);

  void nimGossipNodeOnValidated(bool isValid, const uint8_t* keyGuardSignature, size_t keyGuardSignatureSize);
}
