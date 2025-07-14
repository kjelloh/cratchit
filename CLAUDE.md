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
- **Pure TEA Message Handling**: Single `update(msg)` entry point per state - no dispatch chain complexity
- **Modern Option Architecture**: States use `KeyToFunctionOptionsT<F>` template for type-safe options
- **Safe Lambda Captures**: Options and UX created when state is fully constructed, eliminating capture issues
- **Application-Level Input Processing**: Keyboard input and common commands handled in main `cratchit::update()`
- Specialized states: `HADState`, `ProjectState`, `WorkspaceState`, `VATReturnsState`, `FiscalPeriodState`, `HADsState`, etc.

#### Fiscal Framework (`src/fiscal/`):
- `BASFramework`: Swedish BAS (Basic Accounting Standard) integration  
- `SKVFramework`: Swedish Tax Agency integration
- `AmountFramework`: Financial amount handling and calculations
- `HADFramework`: HAD (Heading-Amount-Date) transaction processing

#### Core Processing:
- `cratchit.cpp/h`: Main application logic and TEA implementation with `UserInputBufferState` for text entry
- `cmd.cpp/hpp`: Command processing and execution
- `msg.cpp/hpp`: Message handling system with cargo message types (`CargoMsgT<T>`)
- `tokenize.cpp/hpp`: Input parsing and tokenization

#### Message-Based Cargo System:
Modern message-passing architecture replaces visit/apply pattern:
- **CargoMsgT<T>**: Template-based message wrapper for type-safe payload transport
- **PopStateMsg**: State navigation with optional cargo payload
- **PushStateMsg**: State stack push with new state
- **UserEntryMsg**: String-based user input messages
- **NCursesKeyMsg**: Keyboard input converted to messages at application level

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

#### Update Options Architecture:
- **Modern Approach**: Update options return `StateUpdateResult` with optional state changes and commands
- **Clean Implementation**: Legacy cmd options and dispatch chains removed for pure TEA compliance
- **Template Foundation**: `KeyToFunctionOptionsT<OptionUpdateFunction>` provides type-safe option handling

### Key Features:
- SIE file parsing and processing
- Swedish tax return preparation (SKV forms)
- Interactive state-driven console interface with NCurses
- Financial calculations and account management
- Integration with Swedish fiscal standards (BAS, SKV)
- Persistent environment storage with file-based caching
- HAD (Heading-Amount-Date) transaction processing and input
- Real-time user input handling with `UserInputBufferState` (backspace, commit via Enter)
- Lazy-loaded UI components for optimal performance
- Pure TEA message flow with application-level input processing

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
- **Pure TEA Compliance**: Single `Model → update(msg) → Model` flow with no dispatch chains
- **Immutable State**: States are copied-on-write with shared_ptr for efficiency  
- **Message-Driven**: All input converted to messages, processed through clean TEA update loop
- **Lazy Evaluation**: Options and UX created on-demand, cached for performance
- **Safe Construction**: Constructors focus on data, presentation generated post-construction
- **Template Safety**: Type-safe option handling through template-based architecture
- **Separation of Concerns**: State logic, application logic, and input logic clearly separated
- **Application-Level Input**: Keyboard processing and common commands ('q', '-') handled in main update loop

### Performance Optimizations:
- **Caching**: Options and UX cached after first generation
- **Lazy Loading**: Content only created when actually accessed
- **Shared State**: Immutable states shared via shared_ptr
- **Template Optimization**: Compile-time type safety with runtime efficiency

The application operates as an interactive console where users navigate through different states to perform bookkeeping operations, process tax returns, and manage financial data according to Swedish accounting standards. The architecture follows strict TEA (The Elm Architecture) principles with pure message-driven updates, modern lazy-evaluation patterns, and clean separation between state-specific logic and application-level input processing, ensuring both performance and safety through proper architectural boundaries.

### TEA Implementation Details:

#### Message Flow:
1. **Input Capture**: NCurses/keyboard input converted to `NCursesKeyMsg` 
2. **User Buffer**: `UserInputBufferState` handles text entry (printable chars, backspace, Enter)
3. **State Processing**: Each state's `update(msg)` handles only its specific message types
4. **Application Processing**: `cratchit::update()` handles keyboard shortcuts via `update_options().apply(key)`
5. **Common Commands**: Global commands ('q' quit, '-' back) processed at application level

#### Current Message Types:
- `NCursesKeyMsg`: Keyboard input (char)
- `UserEntryMsg`: Committed text entry (string)  
- `CargoMsgT<EditedItem<HAD>>`: HAD item changes
- `CargoMsgT<HeadingAmountDateTransEntries>`: HAD collections
- `CargoMsgT<EnvironmentPeriodSlice>`: Environment updates
- `PushStateMsg`: Navigate to new state
- `PopStateMsg`: Return to previous state (with optional cargo)

#### State Responsibilities:
- **States**: Handle domain-specific messages only (`EditedHADMsg`, `HADsMsg`, etc.)
- **Application**: Handle input processing, navigation, and global commands
- **User Buffer**: Handle text entry separately from state navigation

This creates a clean, testable architecture where each component has a single, well-defined responsibility.