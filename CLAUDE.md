# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Showtime is a C++23 dataflow network library for connecting performers (clients) through a central stage (server). It enables distributed entity hierarchies, plug-based data connections, and real-time message passing between networked nodes.

## Build Commands

### Installing Dependencies (Windows)
```powershell
# Install all dependencies (libzmq, czmq, flatbuffers, boost)
./install_dependencies.ps1 -config @("debug", "release")

# Skip boost if already installed
./install_dependencies.ps1 -without_boost
```

### Installing Dependencies (Linux/macOS)
Dependencies can be installed via package managers (vcpkg) or built from source. Required:
- libzmq and czmq (with draft API enabled)
- FlatBuffers
- Boost (1.86+ recommended)

### Building
```bash
# Configure (from build directory)
cmake .. -DCMAKE_PREFIX_PATH=<dependency_install_path>

# Build
cmake --build . --config Release

# Build with tests
cmake .. -DBUILD_TESTING=ON
cmake --build . --config Release
```

### Running Tests
```bash
# Run all tests via CTest
ctest -C Release

# Run tests matching a pattern
ctest -C Release -R TestStartup

# Run a single test directly (Windows)
./bin/Windows/x64/TestURI.exe
./bin/Windows/x64/TestStartup.exe

# Run specific test cases within a test file (Boost.Test)
./bin/Windows/x64/TestStartup.exe --run_test=init
./bin/Windows/x64/TestGraph.exe --run_test=connect_cable
```

Test files are in `tests/` and use Boost.Test framework.

### Language Bindings
Enable via CMake options:
- `-DBINDINGS_DOTNET=ON` - C#/.NET
- `-DBINDINGS_PYTHON=ON` - Python (requires matching Python version)
- `-DBINDINGS_JAVA=ON` - Java
- `-DBINDINGS_UNITY=ON` - Unity package
- `-DBINDINGS_UNREAL=ON` - Unreal plugin

## Architecture

### Core Concepts

**Performer** (`ZstPerformer`): A client node that owns entities and connects to the stage. Each client has a root performer that contains its entity hierarchy.

**Stage** (`ZstStage`): The central server that coordinates performers, routes messages, and manages the global entity hierarchy.

**Entity Hierarchy**: Tree structure where entities (`ZstEntityBase`) can contain child entities. Types include:
- `ZstComponent` - Container for plugs
- `ZstPlug` - Data endpoint (input or output)
- `ZstEntityFactory` - Creates entities dynamically

**Plugs and Cables**: `ZstOutputPlug` sends data, `ZstInputPlug` receives. `ZstCable` connects an output to an input for dataflow.

### Module Structure

```
src/core/      - Shared core: hierarchy, session, transports, events
src/client/    - Client implementation (ZstClient, ZstClientSession)
src/server/    - Server implementation (ZstStage, ZstStageSession)
src/plugins/   - Loadable plugin system (entity factories)
include/       - Public API headers
schemas/       - FlatBuffers message definitions
bindings/      - SWIG-based language bindings
```

### Key Classes

- `ShowtimeClient` / `ShowtimeServer` - Public API entry points
- `ZstHierarchy` - Entity tree management and lookup
- `ZstSession` - Cable management and compute triggering
- `ZstSynchronisable` - Base for all synchronized objects (entities, cables)
- `ZstEventDispatcher<T>` - Type-safe event system for adaptors

### Adaptor Pattern

The library uses adaptors for event handling:
- `ZstConnectionAdaptor` - Connection lifecycle events
- `ZstHierarchyAdaptor` - Entity arriving/leaving events
- `ZstSessionAdaptor` - Cable created/destroyed events
- `ZstEntityAdaptor` - Per-entity events

Register adaptors via `add_*_adaptor()` methods on `ShowtimeClient`.

### Transport Layer

Uses ZeroMQ (CZMQ) for networking:
- `ZstZMQClientTransport` / `ZstZMQServerTransport` - Reliable stage communication
- `ZstTCPGraphTransport` - Reliable plug data (TCP)
- `ZstUDPGraphTransport` - Unreliable plug data (UDP)
- `ZstServiceDiscoveryTransport` - Server beacon discovery

### Synchronisation Lifecycle

`ZstSynchronisable` is the base class for all networked objects (entities, cables). Key states:
- `CREATED` → `ACTIVATION_QUEUED` → `ACTIVATING` → `ACTIVATED` (registered with stage)
- `DEACTIVATION_QUEUED` → `DEACTIVATING` → `DESTROYED` (removed from stage)

Entities must be activated before they can be used. Deactivation propagates to child entities.

### Serialization

FlatBuffers for message serialization. Schemas in `schemas/messaging/`:
- `stage_message.fbs` - Client-server protocol
- `graph_message.fbs` - Plug value transport
- `graph_types.fbs` - Entity/plug/cable definitions
- `session.fbs` - Session state serialization

Generated headers go to `build/include/showtime/schemas/`.

## Dependencies

- Boost (log, thread, fiber, coroutine, context, chrono, test)
- ZeroMQ + CZMQ
- FlatBuffers
- SWIG (for bindings)

## Build Options

Key CMake options:
- `BUILD_SHARED_FROM_STATIC_LIBS=ON` (default) - Single shared library combining all modules
- `BUILD_STATIC=ON` - Build separate static libraries
- `BUILD_SHARED=ON` - Build separate shared libraries
- `BUILD_SERVER_LAUNCHER=ON` - Build server executable
- `BUILD_TESTING=ON` - Enable tests
