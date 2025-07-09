# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Development Commands

This is a C++23 project using CMake and Conan package manager.

### Initial setup:
```bash
./init_tool_chain.zsh        # Initialize toolchain (conan install + cmake preset)
```

### Build commands:
```bash
cmake --build build/Release  # Build from project root
./run.zsh                    # Build and run in workspace/ directory
```

### Test commands:
Tests are integrated into the main executable. Run with `--nop` flag to execute tests.

## Architecture Overview

Cratchit is a Swedish bookkeeping console application that processes SIE (Standard Import Export) format files and integrates with SKV (Swedish Tax Agency) frameworks.

### Core Architecture:
- **TEA Pattern**: The application follows The Elm Architecture pattern with `init()`, `update()`, and `view()` functions
- **State Machine**: Built around a state-based UI system with different states for various operations
- **Two Main Namespaces**:
  - `first`: Main application logic using TEA pattern
  - `tea`: Runtime and event handling utilities

### Key Components:

#### State System (`src/states/`):
- `StateImpl`: Base state implementation using CmdOption mechanism for TEA-compliant commands
- **Message Dispatch**: Robust `dispatch()` → `update()` → `default_update()` pattern with guaranteed fallback
- **CmdOption Architecture**: States use `add_cmd_option(char, CmdOption)` for navigation options
- **Pattern 1 Design**: Static `cmd_option_from()` methods provide encapsulated option creation
- **Input Processing**: User keyboard input and command entry handled at state level
- Specialized states: `HADState`, `ProjectState`, `WorkspaceState`, `VATReturnsState`, `FiscalPeriodState`, `HADsState`, etc.

#### Fiscal Framework (`src/fiscal/`):
- `BASFramework`: Swedish BAS (Basic Accounting Standard) integration  
- `SKVFramework`: Swedish Tax Agency integration
- `AmountFramework`: Financial amount handling and calculations
- `HADFramework`: HAD (Heading-Amount-Date) transaction processing

#### Core Processing:
- `cratchit.cpp/h`: Main application logic and TEA implementation
- `cmd.cpp/hpp`: Command processing and execution
- `msg.cpp/hpp`: Message handling system
- `tokenize.cpp/hpp`: Input parsing and tokenization

#### Cargo System (`src/cargo/`):
Data transport objects for moving information between components:
- `CargoBase`: Base cargo functionality
- `HADsCargo`: HAD transaction collections
- `EnvironmentCargo`: Environment and workspace data

### State Navigation Hierarchy:
- **FrameworkState**: Root state showing workspace selection
- **WorkspaceState**: Organization/project selection (ITfied AB, Org x, etc.)
- **ProjectState**: Fiscal year/quarter selection with environment persistence
- **FiscalPeriodState**: HAD transactions and VAT returns access
- **HADsState**: HAD collection with Mod10View pagination
- **HADState**: Individual HAD with SIE export and delete options
- **VATReturnsState**: Swedish tax return processing
- **DeleteItemState**: Confirmation dialogs (Yes/No options)

### Key Features:
- SIE file parsing and processing
- Swedish tax return preparation (SKV forms)
- Interactive state-driven console interface with NCurses
- Financial calculations and account management
- Integration with Swedish fiscal standards (BAS, SKV)
- Persistent environment storage with file-based caching
- HAD (Heading-Amount-Date) transaction processing and input
- Real-time user input handling with backspace and command submission

### Dependencies:
- ncurses (console UI)
- spdlog (logging)
- pugixml (XML processing) 
- ICU (internationalization)
- sol2 (Lua integration)
- immer (immutable data structures)
- gtest (testing)

### Build Requirements:
- C++23 compiler (GCC or Clang)
- CMake 3.23+
- Conan package manager

### Command Patterns:
- **Pattern 1 (Navigation)**: `TargetState::cmd_option_from(...)` - Encapsulated state creation with captions
- **Pattern 3 (Actions)**: Inline lambda commands for non-navigation operations (delete, confirm, etc.)
- **Common Keys**: `q` (quit), `-` (back), `0-9` (options), text input for HAD entry

### Architecture Principles:
- **TEA Compliance**: All commands return `Cmd` objects, not direct state mutations
- **Immutable State**: States are copied-on-write with shared_ptr for efficiency
- **Message-Driven**: NCurses input converted to messages, processed through dispatch chain
- **Fallback Safety**: Template Method pattern ensures base functionality always available
- **Encapsulation**: Each state manages its own option captions and navigation logic

The application operates as an interactive console where users navigate through different states to perform bookkeeping operations, process tax returns, and manage financial data according to Swedish accounting standards. The architecture follows strict TEA (The Elm Architecture) principles with immutable state management and command-based side effects.