# p2p-microkernel

### A High-Performance P2P Microkernel for Secure Gossip
*Built with C++, Nim, nim-libp2p, Cap'n Proto, and libsodium*

---

A research implementation of a **capability-based P2P node** using microkernel-style process isolation. The goal is to demonstrate how security-critical and network-facing concerns can be separated at the process boundary — without sacrificing performance.

The core idea: security-sensitive logic (private key access, block validation) lives in an isolated process that never touches the network. Network-facing logic (gossip ingestion) lives in a separate process written in Nim. Neither can directly access the other. The Orchestrator brokers every connection through typed capability references. If the network-facing process is compromised, the validator and its keys are untouched.

---

## Why this exists

Most examples of Cap'n Proto RPC, Unix process isolation, and Ed25519 signing exist separately. This project puts them together in one working, buildable system — a reference for how these primitives compose into a real P2P node runtime.

If you are building a decentralized node, a plugin runtime, or anything that needs process isolation with efficient IPC, the patterns here apply directly. The microkernel is not the point — **security by separation** is the point. The microkernel is just how you get there.

---

## Why this architecture

### Why microkernel-style isolation?

In a monolithic process, a vulnerability in the network layer can reach the signing keys. By splitting into isolated processes — each with its own address space and no shared memory by default — we contain the blast radius. The `Validator` holds the private keys and never touches the network. The `NetworkListener` touches the network and never sees the keys. The Orchestrator connects them only when needed, and only through a typed capability interface.

This maps directly to how **L4-style microkernels** work: a minimal core manages process spawning and IPC, everything else runs as isolated user-space services.

### Why Cap'n Proto?

- **Native IPC over Unix socketpairs** — no TCP overhead, no loopback, direct kernel-buffered communication between local processes
- **Capability-based RPC** — a service reference *is* an access token. You cannot call a service you were never given a capability to. This makes the security model explicit in the type system
- **Zero-copy friendly** — Cap'n Proto's wire format is designed to be read in-place without deserialization. Combined with shared memory, it enables a true zero-copy fast path for bulk data
- **Schema versioning and code generation** — no hand-rolled message structs, no silent corruption of binary fields

### Why Nim for the NetworkListener?

The `NetworkListener` is the process exposed to the network. It will eventually use **nim-libp2p** for gossip protocol support — the most complete libp2p implementation outside of Go. Nim gives us:

- Memory safety without a borrow checker — important when writing network-facing code
- Near-C performance with garbage collection tunable for low-latency workloads
- First-class C FFI — calling into C++ is straightforward, which matters because Cap'n Proto has no Nim RPC implementation

The tradeoff: Cap'n Proto RPC stays in C++. The Nim side handles gossip logic and calls into the C++ shim via `extern "C"` FFI. A small sacrifice for the convenience of the language and the power of nim-libp2p.

---

## Architecture

```
┌──────────────────────────────────────────────┐
│                  Orchestrator (C++)          │
│  - spawns and monitors all services          │
│  - holds Validator::Client                   │
│  - holds NetworkListener::Client             │
│  - brokers connectToValidator() requests     │
└────────────┬─────────────────┬───────────────┘
             │  socketpair     │  socketpair
     ┌───────▼──────┐   ┌──────▼──────────────────────┐
     │  Validator   │   │   NetworkListener            │
     │    (C++)     │   │   (Nim logic + C++ RPC shim) │
     │              │   │                              │
     │  - verifies  │   │  - ingests P2P gossip        │
     │    Ed25519   │   │  - signs blocks before       │
     │    block     │   │    forwarding                │
     │    signatures│   │  - verifies validator's      │
     │  - signs     │   │    signed response           │
     │    responses │   └──────────────────────────────┘
     └──────────────┘
```

Communication flow when the NetworkListener receives a gossip packet:

```
NetworkListener  →  orchestrator.connectToValidator()
                 →  validator.validateBlock(data, signature)
                 ←  (isValid :Bool, validatorSignature :Data)
NetworkListener  →  crypto_sign_verify_detached(validatorSignature)
                    ↑ proves the response came from THIS validator and was not tampered with
```

The validator never touches the network. The listener never sees the private key. All connections brokered through the Orchestrator.

---

## IPC — Cap'n Proto schema

```capnp
interface MicroService {
    getName @0 () -> (name :Text);
    ping    @1 () -> ();
}

interface Validator extends(MicroService) {
    validateBlock @0 (data :Data, signature :Data)
                  -> (isValid :Bool, validatorSignature :Data);
}

interface NetworkListener extends(MicroService) {
    startListening @0 (port :UInt16) -> ();
}

interface Orchestrator {
    getServices              @0 () -> (services :List(Text));
    connectToValidator       @1 () -> (validator :Validator);
    connectToNetworkListener @2 () -> (listener :NetworkListener);
}
```

`Data` fields carry raw bytes — no UTF-8 assumptions, no silent corruption of binary signatures.

---

## What's implemented

### Process management
The Orchestrator forks and launches child services using `execl`. Each service gets its own Unix socket file descriptor passed as `argv[1]`. `FD_CLOEXEC` ensures no file descriptors leak across `exec` boundaries.

Services are managed through `ServiceHandle` (RAII, move-only) and `ServicesRegistry` (single source of truth for all running processes). The destructor sends `SIGTERM` and waits — no double-close, no zombie processes.

### Cap'n Proto RPC over socketpairs
Each connection uses a single bidirectional `socketpair`. Both sides use `TwoPartyClient` — the Orchestrator as `Side::CLIENT`, services as `Side::SERVER`. After the handshake both sides are symmetric: either can call the other.

The Orchestrator exports itself as a bootstrap capability to each service. This means:
- The Validator can call `orchestrator.connectToValidator()` — it holds an `Orchestrator::Client`
- The NetworkListener can request a `Validator` cap through the broker and call it directly

`OrchestratorImpl` is constructed empty before services are spawned, then service clients are injected via setters after connections are established — breaking the circular dependency cleanly.

### Ed25519 block validation (libsodium)

**Validator:**
- Holds the NetworkListener's public key — verifies incoming block signatures
- Expands its own 32-byte private key seed into a 64-byte signing key at startup via `crypto_sign_seed_keypair`
- On each `validateBlock`:
  1. `crypto_sign_verify_detached` — verifies the block was signed by the expected listener
  2. Appends `0x01` (valid) or `0x00` (invalid) to the data
  3. `crypto_sign_detached` — signs the result with the validator's private key
  4. Returns `(isValid, validatorSignature)` — tamper-proof

**NetworkListener:**
- Signs each block with its own private key before forwarding
- Verifies the validator's signed response — confirms the result came from the right validator
- Currently sends two hardcoded mock blocks (one valid, one with a corrupted signature) to exercise the full pipeline end-to-end

---

## Project structure

```
├── Orchestrator/
│   ├── Orchestrator.h/.cpp     # OrchestratorImpl — capability broker
│   ├── ServiceConnection.h     # spawnAndConnect() helper
│   └── main.cpp
├── Validator/
│   ├── validator.h/.cpp        # Ed25519 block validation + response signing
│   └── main.cpp
├── Network_Listener/
│   ├── NetworkListener.h/.cpp  # Block signing + validator interaction (C++ RPC shim)
│   └── main.cpp
├── proto/
│   └── orchestrator.capnp      # Cap'n Proto schema
├── ServiceHandle.h/.cpp        # RAII process + socket ownership
├── ServicesRegistry.h/.cpp     # Registry of all running services
├── SharedMemory.h/.cpp         # Shared memory (planned — zero-copy fast path)
└── ipc_common.h                # Legacy IPC structs (kept for reference)
```

---

## Building

```bash
mkdir -p cmake-build-debug && cd cmake-build-debug
cmake ..
cmake --build .
./orchestrator
```

Requires CMake 3.20+, a C++20 compiler, Cap'n Proto, and libsodium:

```bash
brew install capnp libsodium   # macOS
```

---

## What's next

### Phase 1 — Nim NetworkListener (nim-libp2p)
Rewrite the `NetworkListener` gossip logic in **Nim** using **[nim-libp2p](https://github.com/status-im/nim-libp2p)** — the most complete libp2p implementation outside of Go, maintained by the Status/Nimbus team. The C++ `main.cpp` keeps the socket, the `TwoPartyClient`, and the bootstrap handshake. The gossip handling moves into a Nim shared library exposed via `extern "C"` FFI. Cap'n Proto RPC stays in C++; nim-libp2p handles peer discovery, gossip transport, and message propagation in Nim.

### Phase 2 — mDNS local discovery
Integrate **mDNS** into the Orchestrator so each node announces `_logos-node._udp` on the local network. Nodes discover each other without a coordinator — the same bootstrap mechanism libp2p uses for local peer discovery.

### Future
- **Shared memory fast path** — reintroduce `SharedMemory` for bulk data transfer between services. Cap'n Proto handles signalling and capability passing; shared memory handles the data itself. True zero-copy between isolated processes.
- **Fault tolerance** — Orchestrator detects crashed services via `SIGCHLD` and restarts them automatically.

