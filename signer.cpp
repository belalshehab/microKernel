#include <iostream>
#include <unistd.h>

#include "ipc_common.h"

int main(int argc, char* argv[]) {
    std::cout << "[Signer] Service initialized, PID: " << getpid() << "\n";

    if (argc < 2) {
        std::cerr << "[Signer] Error: No socket FD provided\n";
        return 1;
    }

    int socketFD = atoi(argv[1]);
    std::cout << "[Signer] Received socket FD: " << socketFD << "\n";

    std::cout << "[Signer] Service running, PID: " << getpid() << "\n";

    Message connectMessage;
    connectMessage.type = CONNECT_REQUEST;
    connectMessage.payloadSize = snprintf(connectMessage.payload, sizeof(connectMessage.payload), "Signer service ready");

    std::cout << "[Signer] Sending CONNECT_REQUEST to orchestrator...\n";
    if (!sendMessage(socketFD, connectMessage, "Signer")) {
        std::cerr << "[Signer] Failed to send CONNECT_REQUEST\n";
        return 1;
    }
    Message responseMessage;
    if (!receiveMessage(socketFD, responseMessage, "Signer")) {
       std::cerr << "[Signer] Failed to receive CONNECT_RESPONSE\n";
        return 1;
    }

    if (responseMessage.type == CONNECT_RESPONSE) {
        std::cout << "[Signer] Received CONNECT_RESPONSE: " << responseMessage.payload << "\n";
        std::cout << "[Signer] Successfully connected to orchestrator!\n";
    } else {
        std::cerr << "[Signer] Unexpected message type: " << responseMessage.type << "\n";
        return 1;
    }
    std::cout << "[Signer] Ready to send data...\n";

    sleep(2);

    std::cout << "[Signer] (Phase 1: No real work yet)\n";
    std::cout << "[Signer] Service shutting down.\n";

    return 0;
}