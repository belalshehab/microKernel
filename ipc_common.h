//
// Created by Belal Shehab on 17/02/2026.
//

#ifndef MICROKERNEL_IPC_COMMON_H
#define MICROKERNEL_IPC_COMMON_H
#include <unistd.h>
#include <iostream>

enum MessageType {
    CONNECT_REQUEST = 1,
    CONNECT_RESPONSE = 2
};

struct Message {
    MessageType type;
    int payloadSize;
    char payload[256];
};
inline bool sendMessage(int sock_fd, const Message &message, const char* sender) {
    ssize_t sent = write(sock_fd, &message, sizeof(Message));
    if (sent != sizeof(Message)) {
        std::cerr << "[" << sender << "] Failed to send message: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

inline bool receiveMessage(int sock_fd, Message &message, const char* receiver) {
    ssize_t received = read(sock_fd, &message, sizeof(Message));
    if (received != sizeof(Message)) {
        if (received == 0) {
            std::cout << "[" << receiver << "] Connection closed by peer.\n";
        } else if (received < 0) {
            std::cerr << "[" << receiver << "] Failed to receive message: " << strerror(errno) << std::endl;
        } else {
            std::cerr << "[" << receiver << "] Incomplete message received: " << received << " bytes\n";
        }
        return false;
    }
    return true;
}
#endif //MICROKERNEL_IPC_COMMON_H