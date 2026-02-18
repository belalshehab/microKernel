#include <iostream>
#include <unistd.h>

#include "ipc_common.h"

int main(int argc, char* argv[]) {
    std::cout << "[Hasher] Service initialized, PID: " << getpid() << "\n";

    if (argc < 2) {
        std::cerr << "[Hasher] Error: No socket FD provided\n";
        return 1;
    }

    int socketFD = atoi(argv[1]);
    std::cout << "[Hasher] Received socket FD: " << socketFD << "\n";

    std::cout << "[Hasher] Service running, PID: " << getpid() << "\n";

    Message connectMessage;
    connectMessage.type = CONNECT_REQUEST;
    connectMessage.payloadSize = snprintf(connectMessage.payload, sizeof(connectMessage.payload), "Hasher service ready");

    std::cout << "[Hasher] Sending CONNECT_REQUEST to orchestrator...\n";
    if (!sendMessage(socketFD, connectMessage, "Hasher")) {
        std::cerr << "[Hasher] Failed to send CONNECT_REQUEST\n";
        return 1;
    }
    Message responseMessage;
    if (!receiveMessage(socketFD, responseMessage, "Hasher")) {
        std::cerr << "[Hasher] Failed to receive CONNECT_RESPONSE\n";
        return 1;
    }

    if (responseMessage.type == CONNECT_RESPONSE) {
        std::cout << "[Hasher] Received CONNECT_RESPONSE: " << responseMessage.payload << "\n";
        std::cout << "[Hasher] Successfully connected to orchestrator!\n";
    } else {
        std::cerr << "[Hasher] Unexpected message type: " << responseMessage.type << "\n";
        return 1;
    }
    std::cout << "[Hasher] Ready to send data...\n";
    sleep(3);

    std::cout << "[Hasher] (Phase 1: No real work yet)\n";
    std::cout << "[Hasher] Service shutting down.\n";

    return 0;
}
