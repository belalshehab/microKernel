#include <iostream>
#include <unistd.h>

int main(int argc, char* argv[]) {
    std::cout << "[Signer] Service initialized, PID: " << getpid() << "\n";

    if (argc < 2) {
        std::cerr << "[Signer] Error: No socket FD provided\n";
        return 1;
    }

    int socketFD = atoi(argv[1]);
    std::cout << "[Signer] Received socket FD: " << socketFD << "\n";

    std::cout << "[Signer] Service running, PID: " << getpid() << "\n";
    sleep(2);

    std::cout << "[Signer] (Phase 1: No real work yet)\n";
    std::cout << "[Signer] Service shutting down.\n";

    return 0;
}