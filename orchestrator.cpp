#include <iostream>
#include <unistd.h>
#include <signal.h>

pid_t spawn_process(const char* process_name, const char* binary_path) {
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[Orchestrator] Fork failed to fork: " << process_name << ": " << strerror(errno) << std::endl;
        return -1;
    }
    if (pid == 0) {
        // child process
        execl(binary_path, process_name, nullptr);
        // we shouldn't reach here unless exec fails
        std::cerr << "[Orchestrator] Exec failed for " << process_name << ": " << strerror(errno) << std::endl;
        exit(1);
    }
    // parent process
    std::cout << "[Orchestrator] Spawned Process (PID: " << pid << ")\n";
    return pid;
}

int main()
{
    std::cout << "[Orchestrator] Starting microkernel...\n";
    std::cout << "[Orchestrator] PID: " << getpid() << "\n";

    int hasherSocketPair[2];
    int signerSocketPair[2];



    pid_t hasher_pid = spawn_process("hasher", "./hasher");
    if (hasher_pid < 0) {
        return 1;
    }

    pid_t signer_pid = spawn_process("signer", "./signer");
    if (signer_pid < 0) {
        // Clean up hasher if signer fails to spawn
        kill(hasher_pid, SIGTERM);
        waitpid(hasher_pid, nullptr, 0);
        return 1;
    }

    // Parent - orchestrator
    std::cout << "[Orchestrator] Spawned Hasher (PID: " << hasher_pid << ")\n";
    std::cout << "[Orchestrator] Spawned Signer (PID: " << signer_pid << ")\n";
    std::cout << "[Orchestrator] Monitoring child processes...\n";

    // Wait for both children to complete
    int status;
    pid_t finished_pid;
    while ((finished_pid = wait(&status)) > 0) {
        if (finished_pid == hasher_pid) {
            std::cout << "[Orchestrator] Hasher (PID: " << hasher_pid << ") finished with status " << WEXITSTATUS(status) << "\n";
        } else if (finished_pid == signer_pid) {
            std::cout << "[Orchestrator] Signer (PID: " << signer_pid << ") finished with status " << WEXITSTATUS(status) << "\n";
        } else {
            std::cout << "[Orchestrator] Unknown child process (PID: " << finished_pid << ") finished with status " << WEXITSTATUS(status) << "\n";
        }
    }
    return 0;
}