//
// Created by Belal Shehab on 28/02/2026.
//

#include <iostream>
#include <unistd.h>
#include <sodium.h>
#include <capnp/rpc-twoparty.h>
#include "validator.h"
#include "orchestrator.capnp.h"

int main(int argc, char* argv[]) {
    if (sodium_init() < 0) {
        std::cerr << "[Validator] libsodium init failed\n";
        return 1;
    }
    std::cout << "[Validator] Service initialized, PID: " << getpid() << "\n";

    if (argc < 2) {
        std::cerr << "[Validator] Error: No socket FD provided\n";
        return 1;
    }

    int socketFD = atoi(argv[1]);

    auto ioContext = kj::setupAsyncIo();
    auto stream = ioContext.lowLevelProvider->wrapSocketFd(
        socketFD,
        kj::LowLevelAsyncIoProvider::TAKE_OWNERSHIP
    );

    // Step 1: construct the impl, save a raw pointer before moving it
    auto implOwned = kj::heap<ValidatorImpl>("Validator");
    ValidatorImpl* implPtr = implOwned.get();

    // Step 2: construct the RPC client — exports our impl as bootstrap,
    //         imports the orchestrator's bootstrap over the same socket.
    capnp::TwoPartyClient rpc(
        *stream,
        kj::mv(implOwned),
        capnp::rpc::twoparty::Side::SERVER
    );

    // Step 3: rpc is now fully constructed — safe to call bootstrap().
    //         Inject the orchestrator cap into the impl via the setter.
    implPtr->setOrchestrator(rpc.bootstrap().castAs<Orchestrator>());
    std::cout << "[Validator] Got Orchestrator capability — ready to call back.\n";

    // Block until the orchestrator closes the connection
    rpc.onDisconnect().wait(ioContext.waitScope);

    return 0;
}
