#include "mameFSM.hpp"
#include <iostream>

// Simple example that works with the current API constraints
// Each state can only transition to one specific next state

struct Context {
    int counter = 0;
};

struct StateA;
struct StateB;
struct StateC;

// StateA always transitions to StateB
struct StateA {
    void on_enter(Context& ctx) {
        std::cout << "Enter StateA, counter=" << ctx.counter << "\n";
    }
    
    void on_exit(Context&) {
        std::cout << "Exit StateA\n";
    }
    
    auto on_update(Context& ctx) {
        ctx.counter++;
        std::cout << "StateA update, counter=" << ctx.counter << "\n";
        return mameFSM::make_transition<StateB>();
    }
};

// StateB always transitions to StateC
struct StateB {
    void on_enter(Context& ctx) {
        std::cout << "Enter StateB, counter=" << ctx.counter << "\n";
    }
    
    void on_exit(Context&) {
        std::cout << "Exit StateB\n";
    }
    
    auto on_update(Context& ctx) {
        ctx.counter++;
        std::cout << "StateB update, counter=" << ctx.counter << "\n";
        return mameFSM::make_transition<StateC>();
    }
};

// StateC always transitions to StateA
struct StateC {
    void on_enter(Context& ctx) {
        std::cout << "Enter StateC, counter=" << ctx.counter << "\n";
    }
    
    void on_exit(Context&) {
        std::cout << "Exit StateC\n";
    }
    
    auto on_update(Context& ctx) {
        ctx.counter++;
        std::cout << "StateC update, counter=" << ctx.counter << "\n";
        return mameFSM::make_transition<StateA>();
    }
};

int main() {
    Context ctx;
    using SimpleFSM = mameFSM::FSM<Context, StateA, StateB, StateC>;
    SimpleFSM::Runner<StateA> fsm(ctx);
    
    std::cout << "=== Simple Working FSM Demo ===\n\n";
    
    // Run through several state transitions
    for (int i = 0; i < 6; ++i) {
        std::cout << "Update " << i << ":\n";
        fsm.update(ctx);
        std::cout << "\n";
    }
    
    return 0;
}