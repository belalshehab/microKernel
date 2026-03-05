#include <iostream>
#include <unistd.h>
#include <sodium.h>
#include <capnp/rpc-twoparty.h>
#include "GossipNode.h"
#include "orchestrator.capnp.h"

int main(int argc, char* argv[]) {
    if (sodium_init() < 0) {
        std::cerr << "[GossipNode] libsodium init failed\n";
        return 1;
    }
    std::cout << "[GossipNode] Service initialized, PID: " << getpid() << "\n";

    if (argc < 2) {
        std::cerr << "[GossipNode] Error: No socket FD provided\n";
        return 1;
    }

    int socketFD = atoi(argv[1]);

    auto ioContext = kj::setupAsyncIo();
    auto stream = ioContext.lowLevelProvider->wrapSocketFd(
        socketFD,
        kj::LowLevelAsyncIoProvider::TAKE_OWNERSHIP
    );

    auto implOwned = kj::heap<GossipNodeImpl>("GossipNode");
    GossipNodeImpl* implPtr = implOwned.get();

    capnp::TwoPartyClient rpc(
        *stream,
        kj::mv(implOwned),
        capnp::rpc::twoparty::Side::SERVER
    );

    implPtr->setOrchestrator(rpc.bootstrap().castAs<Orchestrator>());
    std::cout << "[GossipNode] Got Orchestrator capability — ready to call back.\n";

    rpc.onDisconnect().wait(ioContext.waitScope);

    return 0;
}
