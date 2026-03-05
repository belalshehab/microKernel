#include <iostream>
#include <unistd.h>
#include <string>

#include "../ServicesRegistry.h"
#include "ServiceConnection.h"
#include "Orchestrator.h"

#include "orchestrator.capnp.h"
#include <capnp/rpc-twoparty.h>

int main() {
    ServicesRegistry services;

    std::cout << "[Orchestrator] Starting microkernel...\n";
    std::cout << "[Orchestrator] PID: " << getpid() << "\n";

    auto ioContext = kj::setupAsyncIo();

    auto orchestratorOwned = kj::heap<OrchestratorImpl>();
    OrchestratorImpl* orchestratorPtr = orchestratorOwned.get();
    Orchestrator::Client orchestratorCap = kj::mv(orchestratorOwned);

    auto keyGuardConn = spawnAndConnect(services, "keyGuard", "../KeyGuard/keyGuard", ioContext, orchestratorCap);
    if (!keyGuardConn) return 1;

    auto gossipNodeConn = spawnAndConnect(services, "gossipNode", "../GossipNode/gossipNode", ioContext, orchestratorCap);
    if (!gossipNodeConn) return 1;

    auto keyGuardClient  = keyGuardConn->getClient<KeyGuard>();
    auto gossipNodeClient = gossipNodeConn->getClient<GossipNode>();

    orchestratorPtr->setKeyGuard(keyGuardConn->getClient<KeyGuard>());
    orchestratorPtr->setGossipNode(gossipNodeConn->getClient<GossipNode>());

    // ── Ping both to confirm liveness ─────────────────────────────────────────
    keyGuardClient.pingRequest().send().wait(ioContext.waitScope);
    std::cout << "[Orchestrator] KeyGuard ping OK\n";

    gossipNodeClient.pingRequest().send().wait(ioContext.waitScope);
    std::cout << "[Orchestrator] GossipNode ping OK\n";

    // ── Tell the GossipNode to start — it will call the KeyGuard itself ────────
    auto startListeningRequest = gossipNodeClient.startListeningRequest();
    startListeningRequest.setPort(12345);
    startListeningRequest.send().wait(ioContext.waitScope);
    std::cout << "[Orchestrator] GossipNode startListening(12345) OK\n";

    kj::NEVER_DONE.wait(ioContext.waitScope);
    std::cout << "[Orchestrator] Done.\n";
    return 0;
}
