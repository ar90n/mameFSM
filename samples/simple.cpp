#include "mameFSM.hpp"
#include <iostream>
#include <string>

struct GameContext {
    int health = 100;
    bool emergency_triggered = false;
    bool game_active = false;
};

// Forward declarations
struct Idle;
struct Active;
struct EmergencyBase;
using Emergency = mameFSM::ForceTransitionable<EmergencyBase>;

// Define transition variant for this FSM
using GameTransition = std::variant<
    mameFSM::Transition<Idle>,
    mameFSM::Transition<Active>,
    mameFSM::Transition<Emergency>
>;

struct Idle {
    void on_enter(GameContext& ctx) {
        std::cout << "-> Idle\n";
        ctx.game_active = false;
    }
    
    void on_exit(GameContext&) {
        std::cout << "<- Idle\n";
    }
    
    auto on_update(GameContext& ctx) -> GameTransition {
        if (ctx.game_active) {
            std::cout << "Starting game...\n";
            return mameFSM::make_transition<Active>();
        }
        return mameFSM::make_transition<Idle>();
    }
};

struct Active {
    void on_enter(GameContext& ctx) {
        std::cout << "-> Active\n";
        ctx.game_active = true;
    }
    
    void on_exit(GameContext&) {
        std::cout << "<- Active\n";
    }
    
    auto on_update(GameContext& ctx) -> GameTransition {
        ctx.health -= 10;
        std::cout << "Health: " << ctx.health << "\n";
        
        if (ctx.health <= 0) {
            ctx.emergency_triggered = true;
            return mameFSM::make_transition<Emergency>();
        }
        return mameFSM::make_transition<Active>();
    }
};

struct EmergencyBase {
    void on_enter(GameContext&) {
        std::cout << "-> EMERGENCY!\n";
    }
    
    void on_exit(GameContext&) {
        std::cout << "<- Emergency\n";
    }
    
    auto on_update(GameContext& ctx) -> GameTransition {
        std::cout << "Emergency mode - restoring health\n";
        ctx.health = 100;
        ctx.emergency_triggered = false;
        return mameFSM::make_transition<Idle>();
    }
};

int main() {
    GameContext ctx;
    using GameFSM = mameFSM::FSM<GameContext, Idle, Active, Emergency>;
    GameFSM::Runner<Idle> fsm(ctx);
    
    std::cout << "=== Simple FSM Demo ===\n";
    
    std::cout << "\n1. Initial state (Idle):\n";
    fsm.update(ctx);
    
    std::cout << "\n2. Trigger game start:\n";
    ctx.game_active = true;
    fsm.update(ctx);
    
    std::cout << "\n3. Update Active state (health decreases):\n";
    for (int i = 0; i < 5; ++i) {
        fsm.update(ctx);
    }
    
    std::cout << "\n4. Continue until emergency:\n";
    while (!fsm.is_in_state<Emergency>()) {
        fsm.update(ctx);
    }
    
    std::cout << "\n5. Recover from emergency:\n";
    fsm.update(ctx);
    
    std::cout << "\n6. Check state:\n";
    std::cout << "Is in Idle? " << (fsm.is_in_state<Idle>() ? "Yes" : "No") << "\n";
    std::cout << "Health: " << ctx.health << "\n";
    
    std::cout << "\n7. Force transition test:\n";
    fsm.force_transition<Emergency>(ctx);
    std::cout << "Forced to Emergency? " << (fsm.is_in_state<Emergency>() ? "Yes" : "No") << "\n";
    
    return 0;
}