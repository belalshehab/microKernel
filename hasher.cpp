#include <iostream>
#include <unistd.h>

int main(int argc, char* argv[]) {
    std::cout << "[Hasher] Service initialized, PID: " << getpid() << "\n";

    if (argc < 2) {
        std::cerr << "[Hasher] Error: No socket FD provided\n";
        return 1;
    }

    int socketFD = atoi(argv[1]);
    std::cout << "[Hasher] Received socket FD: " << socketFD << "\n";

    std::cout << "[Hasher] Service running, PID: " << getpid() << "\n";
    sleep(3);

    std::cout << "[Hasher] (Phase 1: No real work yet)\n";
    std::cout << "[Hasher] Service shutting down.\n";

    return 0;
}