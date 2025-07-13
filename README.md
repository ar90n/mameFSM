# mameFSM

[![CI](https://github.com/ar90n/mameFSM/actions/workflows/ci.yml/badge.svg)](https://github.com/ar90n/mameFSM/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C++](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Header Only](https://img.shields.io/badge/header--only-✓-brightgreen.svg)](src/mameFSM.hpp)
![Built with vibe coding](https://img.shields.io/badge/built%20with-vibe%20coding-ff69b4)

A header-only, type-safe hierarchical Finite State Machine (FSM) library for C++23.

## Features

- **Header-only**: Single header file, no dependencies beyond C++23 standard library
- **Type-safe**: Compile-time state validation using C++20 concepts
- **Zero inheritance**: States are simple structs with three required methods
- **Hierarchical FSM**: Natural support for nested state machines
- **Force transitions**: Special decorator for emergency/priority states
- **Minimal boilerplate**: Clean, expressive API

## Requirements

- C++23 compatible compiler (GCC 13+, Clang 16+)
- Standard library with `<concepts>`, `<variant>`, `<optional>` support

## Quick Start

```cpp
#include "mameFSM.hpp"
#include <iostream>

struct Context {
    int counter = 0;
};

// Forward declarations
struct Idle;
struct Active;

// Define transition variant for your FSM
using MyTransition = std::variant<
    mameFSM::Transition<Idle>,
    mameFSM::Transition<Active>
>;

// Define states
struct Idle {
    void on_enter(Context& ctx) { 
        std::cout << "Entering Idle\n"; 
    }
    void on_exit(Context& ctx) { 
        std::cout << "Exiting Idle\n"; 
    }
    auto on_update(Context& ctx) -> MyTransition {
        if (ctx.counter > 0) {
            return mameFSM::make_transition<Active>();
        }
        return mameFSM::make_transition<Idle>();
    }
};

struct Active {
    void on_enter(Context& ctx) { 
        std::cout << "Entering Active\n"; 
    }
    void on_exit(Context& ctx) { 
        std::cout << "Exiting Active\n"; 
    }
    auto on_update(Context& ctx) -> MyTransition {
        ctx.counter--;
        if (ctx.counter <= 0) {
            return mameFSM::make_transition<Idle>();
        }
        return mameFSM::make_transition<Active>();
    }
};

// Create and run FSM
int main() {
    Context ctx;
    using MyFSM = mameFSM::FSM<Context, Idle, Active>;
    MyFSM::Runner<Idle> fsm(ctx);  // Start in Idle state
    
    ctx.counter = 5;
    while (ctx.counter >= 0) {
        fsm.update(ctx);  // Will transition and update states
    }
    
    return 0;
}
```

## API Reference

### State Concept

States must implement three methods:

```cpp
struct MyState {
    void on_enter(Context& ctx);    // Called when entering this state
    void on_exit(Context& ctx);     // Called when leaving this state
    auto on_update(Context& ctx) -> TransitionVariant;   // Called on update, returns transition
};
```

The `on_update` method must return a transition variant (std::variant of all possible transitions). This enables runtime conditional transitions:

```cpp
// Define all possible transitions for your FSM
using MyTransition = std::variant<
    mameFSM::Transition<StateA>,
    mameFSM::Transition<StateB>,
    mameFSM::Transition<StateC>
>;

// Each state's on_update must return this variant type
auto on_update(Context& ctx) -> MyTransition {
    if (condition_a) {
        return mameFSM::make_transition<StateA>();
    } else if (condition_b) {
        return mameFSM::make_transition<StateB>();
    }
    return mameFSM::make_transition<StateC>();
}
```

### FSM Definition

```cpp
using MyFSM = mameFSM::FSM<Context, State1, State2, ...>;
MyFSM::Runner<InitialState> fsm(context);
```

### State Transitions

Transitions are returned from `on_update` as a variant:

```cpp
auto on_update(Context& ctx) -> MyTransition {
    if (condition) {
        return mameFSM::make_transition<NextState>();
    }
    return mameFSM::make_transition<CurrentState>();  // Stay in current state
}
```

Transitions can also carry arguments for the target state's constructor:

```cpp
struct TargetState {
    int value;
    TargetState(int v) : value(v) {}
    // ... state methods ...
};

// In source state:
auto on_update(Context& ctx) -> MyTransition {
    return mameFSM::make_transition<TargetState>(42);  // Pass constructor args
}
```

### Force Transitions

For emergency or high-priority states that can be transitioned to at any time:

```cpp
struct EmergencyBase {
    void on_enter(Context& ctx) { /* ... */ }
    void on_exit(Context& ctx) { /* ... */ }
    auto on_update(Context& ctx) { /* ... */ }
};

using Emergency = mameFSM::ForceTransitionable<EmergencyBase>;

// Usage
fsm.force_transition<Emergency>(ctx);  // Only works for ForceTransitionable states
```

### Checking Current State

```cpp
if (fsm.is_in_state<MyState>()) {
    // Currently in MyState
}
```

## Hierarchical FSM

States can contain their own FSMs:

```cpp
// Define sub-FSM transitions
using SubTransition = std::variant<
    mameFSM::Transition<SubStateA>,
    mameFSM::Transition<SubStateB>
>;

// Define sub-states
struct SubStateA {
    void on_enter(Context& ctx) { /* ... */ }
    void on_exit(Context& ctx) { /* ... */ }
    auto on_update(Context& ctx) -> SubTransition { /* ... */ }
};

using SubFSM = mameFSM::FSM<Context, SubStateA, SubStateB>;

// Parent state with embedded FSM
struct ParentState {
    std::optional<SubFSM::Runner<SubStateA>> sub_fsm;
    
    void on_enter(Context& ctx) {
        sub_fsm.emplace(ctx);  // Create sub-FSM
    }
    
    void on_exit(Context& ctx) {
        sub_fsm.reset();  // Destroy sub-FSM
    }
    
    auto on_update(Context& ctx) -> ParentTransition {
        if (sub_fsm) {
            sub_fsm->update(ctx);  // Update sub-FSM
        }
        // Check conditions and return appropriate transition
        return mameFSM::make_transition<ParentState>();
    }
};
```

## Building

### Using Make

```bash
# Build everything
make all

# Build only samples
make samples

# Run tests
make test

# Format code
make format

# Clean build artifacts
make clean
```

### Include Path

The library header is located at `src/mameFSM.hpp`. Make sure to add the `src` directory to your include path:

```bash
g++ -std=c++23 -Isrc your_program.cpp
```

## Examples

See the `samples/` directory for complete examples:

- `simple.cpp`: Basic FSM with state transitions and force transitions
- `hierarchical.cpp`: Nested FSMs with parent-child state relationships

## License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## Author

Masahiro Wada