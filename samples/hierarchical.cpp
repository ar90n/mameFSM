#include "mameFSM.hpp"
#include <iostream>
#include <string>
#include <optional>

struct Context {
    float speed = 0.0f;
    bool stop_flag = false;
    bool sprint_requested = false;
    bool emergency_stop = false;
};

// Forward declarations
struct Walk;
struct Run;
struct Idle;
struct Moving;
struct EmergencyBase;
using Emergency = mameFSM::ForceTransitionable<EmergencyBase>;

// Define transition variants for sub-FSM
using MoveTransition = std::variant<
    mameFSM::Transition<Walk>,
    mameFSM::Transition<Run>
>;

// Define transition variant for top-level FSM
using TopTransition = std::variant<
    mameFSM::Transition<Idle>,
    mameFSM::Transition<Moving>,
    mameFSM::Transition<Emergency>
>;

struct Walk {
    void on_enter(Context& ctx) {
        std::cout << "  -> Walk\n";
        ctx.speed = 3.0f;
    }
    
    void on_exit(Context&) {
        std::cout << "  <- Walk\n";
    }
    
    auto on_update(Context& ctx) -> MoveTransition {
        std::cout << "  Walking at speed: " << ctx.speed << "\n";
        if (ctx.sprint_requested) {
            return mameFSM::make_transition<Run>();
        }
        return mameFSM::make_transition<Walk>();
    }
};

struct Run {
    void on_enter(Context& ctx) {
        std::cout << "  -> Run\n";
        ctx.speed = 10.0f;
    }
    
    void on_exit(Context&) {
        std::cout << "  <- Run\n";
    }
    
    auto on_update(Context& ctx) -> MoveTransition {
        std::cout << "  Running at speed: " << ctx.speed << "\n";
        ctx.speed -= 0.5f;
        if (ctx.speed <= 3.0f) {
            ctx.sprint_requested = false;
            return mameFSM::make_transition<Walk>();
        }
        return mameFSM::make_transition<Run>();
    }
};

using MoveFSM = mameFSM::FSM<Context, Walk, Run>;

struct Idle {
    void on_enter(Context& ctx) {
        std::cout << "-> Idle\n";
        ctx.speed = 0.0f;
        ctx.stop_flag = false;
    }
    
    void on_exit(Context&) {
        std::cout << "<- Idle\n";
    }
    
    auto on_update(Context& ctx) -> TopTransition {
        std::cout << "Idling...\n";
        if (ctx.emergency_stop) {
            return mameFSM::make_transition<Emergency>();
        }
        if (ctx.sprint_requested || ctx.speed > 0) {
            return mameFSM::make_transition<Moving>();
        }
        return mameFSM::make_transition<Idle>();
    }
};

struct Moving {
    std::optional<MoveFSM::Runner<Walk>> sub;
    
    void on_enter(Context& ctx) {
        std::cout << "-> Moving\n";
        sub.emplace(ctx);
    }
    
    void on_exit(Context&) {
        std::cout << "<- Moving\n";
        sub.reset();
    }
    
    auto on_update(Context& ctx) -> TopTransition {
        if (sub) {
            sub->update(ctx);
            
            if (ctx.stop_flag) {
                std::cout << "Stop requested from sub-state\n";
                return mameFSM::make_transition<Idle>();
            }
            
            if (ctx.emergency_stop) {
                return mameFSM::make_transition<Emergency>();
            }
        }
        return mameFSM::make_transition<Moving>();
    }
};

struct EmergencyBase {
    void on_enter(Context& ctx) {
        std::cout << "-> EMERGENCY STOP!\n";
        ctx.speed = 0.0f;
        ctx.emergency_stop = true;
    }
    
    void on_exit(Context& ctx) {
        std::cout << "<- Emergency\n";
        ctx.emergency_stop = false;
    }
    
    auto on_update(Context& ctx) -> TopTransition {
        std::cout << "Emergency stop active\n";
        ctx.emergency_stop = false;
        return mameFSM::make_transition<Idle>();
    }
};

using Emergency = mameFSM::ForceTransitionable<EmergencyBase>;

int main() {
    Context ctx;
    using TopFSM = mameFSM::FSM<Context, Idle, Moving, Emergency>;
    TopFSM::Runner<Idle> topFSM(ctx);
    
    std::cout << "=== Hierarchical FSM Demo ===\n";
    
    std::cout << "\n1. Initial state (Idle):\n";
    topFSM.update(ctx);
    
    std::cout << "\n2. Trigger transition to Moving:\n";
    ctx.sprint_requested = true;
    topFSM.update(ctx);
    
    std::cout << "\n3. Update Moving state (with Walk sub-state):\n";
    topFSM.update(ctx);
    topFSM.update(ctx);
    
    std::cout << "\n4. Sprint triggers Run in sub-FSM:\n";
    ctx.sprint_requested = true;
    topFSM.update(ctx);
    
    std::cout << "\n5. Continue running (speed decreases):\n";
    for (int i = 0; i < 8; ++i) {
        topFSM.update(ctx);
    }
    
    std::cout << "\n6. Request stop:\n";
    ctx.stop_flag = true;
    topFSM.update(ctx);
    
    std::cout << "\n7. Back in Idle:\n";
    topFSM.update(ctx);
    
    std::cout << "\n8. Force emergency:\n";
    topFSM.force_transition<Emergency>(ctx);
    topFSM.update(ctx);
    
    std::cout << "\n9. Recover to Idle:\n";
    topFSM.update(ctx);
    
    return 0;
}