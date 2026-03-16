# Cratchit - Developer Guide

## Overview
Cratchit is an SIE-based bookkeeping helper console application written in modern C++23. SIE is a file encoding format for exchanging bookkeeping data between applications, specifically designed for Swedish bookkeeping requirements.

The application is named after Bob Cratchit, the poor clerk of Scrooge in Charles Dickens' "A Christmas Carol."

## Command Line Arguments

- './cratchit --nop'                Run (no operation)
- './cratchit --test'               Run all tests (google test)
- './cratchit --test --gtest_xxx'   Run tests (passing args to google test)
- './cratchit --gtest_xxx...'       Run google tests (passing provided arguments to google test)

## Helper 'run' script

- './run.zsh --nop'                 Build + move to workspace folder + execute './cratchit --nop'
- './run.zsh ...args...'            Build + move to workspace folder + execute './cratchit ...'

## Known buld problems and fixes

- 'Silent' run of cratchit can indicate 'code signing error' (OS rejects execution)
- Command: 'codesign -s - --force --deep workspace/cratchit' fixes code signing error.

## Project Architecture

Cratchit implements **two distinct architectural modes**:

### 1. Zeroth Mode (`zeroth` namespace)
**Location**: `src/zeroth/main.cpp`

The zeroth mode represents the **original monolithic architecture** of the application:
- **Single File Implementation**: The entire zeroth variant is contained in one large C++ file (`main.cpp`)
- **Monolithic Design**: All functionality is implemented directly in the main function and helper functions
- **Version**: Implements version 0.5 of the application
- **Purpose**: Legacy implementation maintained for reference and comparison
- **Architecture**: Traditional procedural/object-oriented approach without formal architectural patterns

**Key Characteristics**:
- Direct file I/O and processing
- Embedded business logic in main flow
- Minimal separation of concerns
- Uses comprehensive includes for Swedish accounting frameworks (BAS, SKV, HAD)

### 2. First Mode (`first` namespace)
**Location**: `src/cratchit.h` → `src/cratchit.cpp` → TEA Architecture

The first mode represents the **refactored architecture** based on **The Elm Architecture (TEA)**:
- **TEA Pattern**: Implements Model-Update-View architecture
- **C API Interface**: Exposed through `cratchit.h` header
- **Entry Point**: `first::main()` function in `cratchit.cpp`
- **Version**: Implements version 0.6 of the application
- **Architecture**: Functional reactive programming pattern

**TEA Components**:
- **Model** (`src/model.hpp`): Application state representation
- **Update** (`src/cratchit_tea.cpp`): State transitions and message handling
- **View** (`src/tea/ncurses_head.cpp`): UI rendering using ncurses
- **Messages** (`src/msgs/`): Command/message system for state changes
- **States** (`src/states/`): Individual UI state implementations

**Key TEA Functions**:
- `init()` - Initialize application state
- `update(Model, Msg) -> ModelUpdateResult` - Handle state transitions
- `view(Model) -> View` - Render current state
- `CratchitRuntime` - TEA runtime that orchestrates the cycle

## Project Structure

```
src/
├── zeroth/
│   └── main.cpp          # Complete zeroth mode implementation
├── cratchit.h            # C API header for first mode
├── cratchit.cpp          # First mode entry point
├── cratchit_tea.cpp      # TEA implementation (init/update/view)
├── tea/                  # TEA runtime and UI components
│   ├── ncurses_head.cpp  # ncurses-based view implementation
│   ├── runtime.hpp       # TEA runtime orchestration
│   └── ...
├── states/               # UI state machines for first mode
├── msgs/                 # Message system for TEA
├── fiscal/               # Swedish accounting frameworks
│   ├── BASFramework.hpp  # BAS (Bokföring och Revision Service)
│   ├── SKVFramework.hpp  # SKV (Skatteverket - Tax Agency)
│   └── ...
└── model.hpp             # TEA model definition
```

## Dependencies & Build System

**Build System**: CMake with Conan package management
**C++ Standard**: C++23 (enforced in CMakeLists.txt and conanfile.py)
**Key Dependencies**:
- `sol2` - Lua scripting integration
- `ncurses` - Terminal UI
- `pugixml` - XML parsing
- `icu` - Unicode support
- `immer` - Immutable data structures
- `spdlog` - Logging
- `GTest` - Testing framework

**Build Commands**:
```bash
./run.zsh --nop    # Primary build-and-run (do nothing) script
cmake --build .    # Direct CMake build
```

## Swedish Accounting Compliance

The application complies with Swedish Accounting Act (Bokföringslagen 1999:1078) Chapter 5, Section 7 requirements:
- Date of business transaction
- Date of voucher compilation
- Voucher number/identification
- Transaction description
- Amount and VAT
- Counterparty name

## Key Frameworks

1. **BAS Framework** (`fiscal/BASFramework.hpp`) - Bokföring och Revision Service
2. **SKV Framework** (`fiscal/SKVFramework.hpp`) - Skatteverket integration
3. **HAD Framework** (`fiscal/amount/HADFramework.hpp`) - Transaction handling
4. **SIE Framework** (`sie/sie.hpp`) - SIE file format processing

## Development Notes

**Current Focus**: The project is actively developed using the **first mode (TEA architecture)**. The zeroth mode is maintained for reference but new features are implemented in the first mode.

**Logging**: Uses spdlog with business-specific logging (`logger::business()`)

**Testing**: Comprehensive test suite in `src/test/` with integration and unit tests

**Character Encoding**: Robust Unicode support via ICU library and custom encoding detection

## Getting Started

1. **For New Features**: Work with the first mode architecture (`src/cratchit.cpp` + TEA components)
2. **For Understanding Legacy Code**: Reference the zeroth mode (`src/zeroth/main.cpp`)
3. **Build**: Use `./run.zsh --nop` for development builds
4. **Architecture**: Follow TEA patterns - state changes via messages, immutable model updates

The first mode provides a clean separation of concerns and maintainable architecture compared to the monolithic zeroth implementation.