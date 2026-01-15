# R-Type - Multiplayer Game Engine

<div align="center">

![R-Type Logo](https://img.shields.io/badge/R--Type-Game%20Engine-blue?style=for-the-badge)
[![C++](https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=c%2B%2B)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.20+-064F8C?style=for-the-badge&logo=cmake)](https://cmake.org/)
[![UDP](https://img.shields.io/badge/Protocol-UDP-green?style=for-the-badge)](https://en.wikipedia.org/wiki/User_Datagram_Protocol)

*A networked multiplayer shoot'em up game with custom game engine*

[Features](#features) • [Installation](#installation) • [Usage](#usage) • [Documentation](#documentation) • [Architecture](#architecture)

</div>

---

## 📖 About

R-Type is a multiplayer networked implementation of the classic horizontal shoot'em up game, built from scratch with a custom game engine. This project demonstrates advanced C++ development techniques, networked game architecture, and modern software engineering practices.

The game features:
- **Multiplayer support**: Up to 4 players fighting together against alien forces
- **Server authority**: Authoritative server architecture ensuring fair gameplay
- **Custom ECS engine**: Entity-Component-System architecture for flexible game logic
- **UDP networking**: Low-latency real-time communication
- **Cross-platform**: Runs on both Linux and Windows

## ✨ Features

### Game Features
- 🎮 **4-player cooperative multiplayer**
- 🚀 **Classic R-Type gameplay** with spaceships, enemies, and projectiles
- 🌌 **Scrolling star-field background**
- 👾 **Multiple enemy types** with varied behaviors
- 💥 **Collision detection** and physics
- 🎵 **Sound effects and music**
- 🏆 **Score tracking** and statistics

### Technical Features
- ⚡ **60 FPS game loop** with fixed timestep physics
- 📡 **20 Hz network snapshots** for bandwidth optimization
- 🧩 **Modular architecture** with decoupled systems
- 🔒 **Thread-safe networking** with producer-consumer pattern
- 🎯 **Binary protocol** with packet validation
- 🔄 **Automatic reconnection** and timeout detection
- 🛡️ **Robust error handling** and validation

## 🚀 Quick Start

### Prerequisites

- **C++ Compiler**: GCC 9+ / Clang 10+ / MSVC 2019+
- **CMake**: 3.20 or higher
- **Package Manager**: Conan, Vcpkg, or CMake CPM
- **Git**: For cloning the repository

### Installation

```bash
# Clone the repository
git clone https://github.com/xxx/r-type.git
cd r-type

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build . --j4

# Binaries are in build/server/ & build/game/
```

### Running the Game

**Start the server:**
```bash
./r-type_server
# Default port: 4242
```

**Start a client:**
```bash
./r-type_client
```

## 🎮 How to Play

### Controls
- **Arrow Keys**: Move your spaceship
- **Space**: Shoot
- **Escape**: Exit game

### Objective
Fight waves of alien enemies cooperatively with up to 3 other players. Destroy enemies to earn points and survive as long as possible!

## 🏗️ Architecture

### System Overview

```
┌─────────────────────────────────────────┐
│          SERVER (Orchestration)         │
│  • Game Loop (60 FPS)                   │
│  • Message Dispatching                  │
│  • Client Management                    │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│       NETWORK MODULE (UDP Layer)        │
│  • Non-blocking Reception Thread        │
│  • Thread-safe Message Queue            │
│  • Packet Validation                    │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│       GAME MODULE (ECS Engine)          │
│  • Entities & Components                │
│  • Systems (Movement, Collision, etc.)  │
│  • Snapshot Generation                  │
└─────────────────────────────────────────┘
```

### Key Components

- **Server**: Authoritative game logic, client management, snapshot broadcasting
- **Client**: Rendering, input handling, state interpolation
- **NetworkModule**: UDP transport, packet validation, thread-safe queuing
- **GameModule**: ECS-based game logic, physics, collision detection
- **Protocol**: Binary UDP protocol with comprehensive packet types

## 📚 Documentation

Comprehensive documentation is available in multiple formats:

- **[Server Architecture](documentation/ServerArchitecture.md)**: Detailed server design and implementation
- **[Protocol RFC](documentatio.md)**: Complete network protocol specification
- **Developer Guide**: How to extend the engine and add features


## 🔧 Configuration

### Server Configuration

The server can be configured through command-line arguments:

```bash
./r-type_server
```

### Network Settings

- **Port**: Default 8080 (UDP)
- **Max Players**: 4 simultaneous players
- **Tick Rate**: 60 Hz game loop
- **Snapshot Rate**: 20 Hz network updates
- **Timeout**: 10 seconds client timeout

## 🧪 Testing

```bash

```

## 🛠️ Development

### Project Structure

```
r-type/
├── server/           # Server implementation
│   ├── Server.cpp/hpp
│   ├── NetworkModule.cpp/hpp
|   ├── GameModule.cpp/hpp
│   └── Protocol.hpp
|   
├── game/           # Client implementation
│   ├── Client.cpp/hpp
│   └── NetworkClient.cpp/hpp
├── engine/           # Shared code
│
│   ├── ECS/          # Entity-Component-System
│   └── Utils/
├── assets/           # Game assets (sprites, sounds)
├── documentation/             # Documentation
├── tests/            # Unit tests
└── CMakeLists.txt    # Build configuration
```

### Build System

The project uses **CMake** with a package manager (Conan/Vcpkg/CPM) for dependency management:

- **SFML**: Graphics, audio, input
- **Asio** (optional): Networking
- **GoogleTest**: Unit testing

### Coding Standards

- **C++17** standard
- Modern C++ practices (smart pointers, RAII, const-correctness)
- **No raw pointers** except when interfacing with C APIs
- Thread-safe code with RAII-style locking
- Comprehensive error handling

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Follow the coding standards
4. Write tests for new features
5. Update documentation
6. Commit your changes (`git commit -m 'Add amazing feature'`)
7. Push to the branch (`git push origin feature/amazing-feature`)
8. Open a Pull Request

### Development Workflow

- Use **feature branches** for development
- **Code review** required for all PRs
- **CI/CD** automatically builds and tests PRs
- Follow **conventional commits** format

## 🐛 Troubleshooting

### Common Issues

**Server won't start:**
- Check if port 4242 is already in use
- Verify firewall settings allow UDP traffic
- Ensure you have permissions to bind to the port

**Client can't connect:**
- Verify server IP and port
- Check network connectivity (`ping server_ip`)
- Ensure firewall allows outgoing UDP connections

**Game lag or stutter:**
- Check network latency (`ping server_ip`)
- Verify CPU usage isn't maxed out
- Try reducing graphics quality settings

**Compilation errors:**
- Ensure all dependencies are installed
- Check CMake version (3.20+ required)
- Verify C++17 compiler support

## 📝 License

This project is part of an educational curriculum at EPITECH.

## 👥 Authors

**R-Type Development Team**
- [Clément Bouret] - Network Architecture
- [Julien Mars] - Game Engine
- [Mathis Jusy & Kilyan Touat] - Client Implementation & Windows crossplatform
- [Yanis Asselman] - Menu & Sound Design

## 🙏 Acknowledgments

- **IREM**: Original R-Type game creators
- **EPITECH**: Project framework and guidance
- **SFML Community**: Graphics library support
- **Game Networking Resources**: Various articles and tutorials


<div align="center">

**[⬆ Back to Top](#r-type---multiplayer-game-engine)**

</div>