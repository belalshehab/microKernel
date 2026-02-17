#include <iostream>
#include <unistd.h>

int main() {
    std::cout << "[Signer] Service initialized, PID: " << getpid() << "\n";

    std::cout << "[Signer] Service running, PID: " << getpid() << "\n";
    sleep(2);

    std::cout << "[Signer] (Phase 1: No real work yet)\n";
    std::cout << "[Signer] Service shutting down.\n";

    return 0;
}