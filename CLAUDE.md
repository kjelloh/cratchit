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
- **Lazy Evaluation**: Modern lazy-loading architecture for UI options and UX content
- **Two Main Namespaces**:
  - `first`: Main application logic using TEA pattern
  - `tea`: Runtime and event handling utilities

### Key Components:

#### State System (`src/states/`):
- `StateImpl`: Base state implementation with lazy-evaluated options and UX system
- **Lazy Update Options**: States define `create_update_options()` for on-demand option generation
- **Lazy UX Generation**: States define `create_ux()` for on-demand UI content creation
- **Message Dispatch**: Robust `dispatch()` → `update()` → `default_update()` pattern with guaranteed fallback
- **Modern Option Architecture**: States use `KeyToFunctionOptionsT<F>` template for type-safe options
- **Safe Lambda Captures**: Options and UX created when state is fully constructed, eliminating capture issues
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

### Modern State Architecture:

#### Lazy Option System:
- **Transient Caching**: `mutable std::optional<UpdateOptions> m_transient_maybe_update_options`
- **Virtual Creation**: States override `create_update_options()` to define behavior
- **Safe Captures**: Options created when state is fully constructed, preventing stale `this` pointers
- **Template-Based**: `KeyToFunctionOptionsT<OptionUpdateFunction>` provides type-safe option handling

#### Lazy UX System:
- **Transient Caching**: `mutable std::optional<UX> m_transient_maybe_ux`
- **Virtual Creation**: States override `create_ux()` to define UI content
- **Dynamic Content**: UX generated based on current state data (environment status, errors, etc.)
- **Constructor Separation**: UX generation completely separated from constructor logic

#### Update Options vs Legacy Cmd Options:
- **Modern Approach**: Update options return `StateUpdateResult` with optional state changes and commands
- **Legacy Support**: Old cmd options architecture commented out but preserved for reference
- **Unified Interface**: Both systems use same `KeyToFunctionOptionsT` template foundation

### Key Features:
- SIE file parsing and processing
- Swedish tax return preparation (SKV forms)
- Interactive state-driven console interface with NCurses
- Financial calculations and account management
- Integration with Swedish fiscal standards (BAS, SKV)
- Persistent environment storage with file-based caching
- HAD (Heading-Amount-Date) transaction processing and input
- Real-time user input handling with backspace and command submission
- Lazy-loaded UI components for optimal performance

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

### Modern Design Patterns:

#### State Construction Pattern:
```cpp
// Constructor focuses only on core data initialization
MyState::MyState(Data data) : StateImpl({}), m_data{data} {
    // NO UX manipulation
    // NO option initialization
}

// Lazy option creation when needed
StateImpl::UpdateOptions MyState::create_update_options() const override {
    UpdateOptions result{};
    result.add('x', {"Action", [this]() -> StateUpdateResult {
        // Safe 'this' capture - state is fully constructed
        return {new_state, command};
    }});
    return result;
}

// Lazy UX creation when needed  
StateImpl::UX MyState::create_ux() const override {
    UX result{};
    result.push_back(std::format("Status: {}", m_data.status));
    return result;
}
```

#### Navigation Pattern:
- **Update Options**: Navigation through `StateUpdateResult` returning new states via `PushStateMsg`
- **Factory Functions**: Static factories for creating states with proper data
- **Template Safety**: `KeyToFunctionOptionsT<F>` ensures type-safe option handling

#### Architecture Principles:
- **TEA Compliance**: All commands return `Cmd` objects, not direct state mutations
- **Immutable State**: States are copied-on-write with shared_ptr for efficiency
- **Message-Driven**: NCurses input converted to messages, processed through dispatch chain
- **Lazy Evaluation**: Options and UX created on-demand, cached for performance
- **Safe Construction**: Constructors focus on data, presentation generated post-construction
- **Template Safety**: Type-safe option handling through template-based architecture
- **Separation of Concerns**: Data initialization, option creation, and UX generation are separate phases

### Performance Optimizations:
- **Caching**: Options and UX cached after first generation
- **Lazy Loading**: Content only created when actually accessed
- **Shared State**: Immutable states shared via shared_ptr
- **Template Optimization**: Compile-time type safety with runtime efficiency

The application operates as an interactive console where users navigate through different states to perform bookkeeping operations, process tax returns, and manage financial data according to Swedish accounting standards. The architecture follows strict TEA (The Elm Architecture) principles with modern lazy-evaluation patterns, ensuring both performance and safety through separation of construction and presentation concerns.