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

    auto  orchestratorOwned = kj::heap<OrchestratorImpl>();
    OrchestratorImpl* orchestratorPtr = orchestratorOwned.get();
    Orchestrator::Client orchestratorCap = kj::mv(orchestratorOwned);

    auto validatorConn = spawnAndConnect(services, "validator", "../Validator/validator", ioContext, orchestratorCap);
    if (!validatorConn) return 1;

    auto listenerConn = spawnAndConnect(services, "networkListener", "../Network_Listener/networkListener", ioContext, orchestratorCap);
    if (!listenerConn) return 1;

    auto validatorClient = validatorConn->getClient<Validator>();
    auto listenerClient  = listenerConn->getClient<NetworkListener>();

    orchestratorPtr->setValidator(validatorConn->getClient<Validator>());
    orchestratorPtr->setListener(listenerConn->getClient<NetworkListener>());

    // ── Ping both to confirm liveness ─────────────────────────────────────────
    validatorClient.pingRequest().send().wait(ioContext.waitScope);
    std::cout << "[Orchestrator] Validator ping OK\n";

    listenerClient.pingRequest().send().wait(ioContext.waitScope);
    std::cout << "[Orchestrator] NetworkListener ping OK\n";

    // ── Tell the listener to start — it will call the validator itself ─────────
    auto startListeningRequest = listenerClient.startListeningRequest();
    startListeningRequest.setPort(12345);
    startListeningRequest.send().wait(ioContext.waitScope);
    std::cout << "[Orchestrator] NetworkListener startListening(12345) OK\n";

    std::cout << "[Orchestrator] Done.\n";

    return 0;
}
