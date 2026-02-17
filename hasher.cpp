#include <iostream>
#include <unistd.h>

int main() {
    std::cout << "[Hasher] Service initialized, PID: " << getpid() << "\n";

    std::cout << "[Hasher] Service running, PID: " << getpid() << "\n";
    sleep(3);

    std::cout << "[Hasher] (Phase 1: No real work yet)\n";
    std::cout << "[Hasher] Service shutting down.\n";

    return 0;
}