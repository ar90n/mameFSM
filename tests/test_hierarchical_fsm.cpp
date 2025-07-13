#include "utest.h"
#include "mameFSM.hpp"
#include <optional>

struct HierContext {
    float speed = 0.0f;
    bool walking = false;
    bool running = false;
    int parent_state = 0;
    int sub_state = 0;
    bool should_run = false;
};

struct SubWalk;
struct SubRun;
struct IdleState;
struct MovingState;

// Define transition variants
using SubTransition = std::variant<
    mameFSM::Transition<SubWalk>,
    mameFSM::Transition<SubRun>
>;

using ParentTransition = std::variant<
    mameFSM::Transition<IdleState>,
    mameFSM::Transition<MovingState>
>;

struct SubWalk {
    void on_enter(HierContext& ctx) {
        ctx.walking = true;
        ctx.running = false;
        ctx.speed = 3.0f;
        ctx.sub_state = 1;
    }
    
    void on_exit(HierContext& ctx) {
        ctx.walking = false;
    }
    
    auto on_update(HierContext& ctx) -> SubTransition {
        ctx.speed += 0.1f;
        if (ctx.should_run) {
            return mameFSM::make_transition<SubRun>();
        }
        return mameFSM::make_transition<SubWalk>();
    }
};

struct SubRun {
    void on_enter(HierContext& ctx) {
        ctx.walking = false;
        ctx.running = true;
        ctx.speed = 10.0f;
        ctx.sub_state = 2;
    }
    
    void on_exit(HierContext& ctx) {
        ctx.running = false;
    }
    
    auto on_update(HierContext& ctx) -> SubTransition {
        ctx.speed -= 0.5f;
        if (ctx.speed <= 3.0f) {
            return mameFSM::make_transition<SubWalk>();
        }
        return mameFSM::make_transition<SubRun>();
    }
};

using SubFSM = mameFSM::FSM<HierContext, SubWalk, SubRun>;

struct IdleState {
    void on_enter(HierContext& ctx) {
        ctx.parent_state = 1;
        // Don't reset speed here as it's used for transition logic
    }
    
    void on_exit(HierContext&) {}
    
    auto on_update(HierContext& ctx) -> ParentTransition {
        if (ctx.speed > 0) {
            return mameFSM::make_transition<MovingState>();
        }
        return mameFSM::make_transition<IdleState>();
    }
};

struct MovingState {
    std::optional<SubFSM::Runner<SubWalk>> sub_fsm;
    
    void on_enter(HierContext& ctx) {
        ctx.parent_state = 2;
        sub_fsm.emplace(ctx);
    }
    
    void on_exit(HierContext& ctx) {
        if (sub_fsm) {
            sub_fsm.reset();
        }
        ctx.sub_state = 0;
        ctx.walking = false;
        ctx.running = false;
    }
    
    auto on_update(HierContext& ctx) -> ParentTransition {
        if (sub_fsm) {
            sub_fsm->update(ctx);
        }
        if (ctx.speed <= 0) {
            return mameFSM::make_transition<IdleState>();
        }
        return mameFSM::make_transition<MovingState>();
    }
};

UTEST(HierarchicalFSM, InitialStateWithSubFSM) {
    HierContext ctx;
    using ParentFSM = mameFSM::FSM<HierContext, IdleState, MovingState>;
    ParentFSM::Runner<IdleState> parent_fsm(ctx);
    
    ASSERT_EQ(ctx.parent_state, 1);
    ASSERT_EQ(ctx.speed, 0.0f);
    
    // Trigger transition to MovingState
    ctx.speed = 1.0f;
    parent_fsm.update(ctx);
    
    ASSERT_EQ(ctx.parent_state, 2);
    ASSERT_EQ(ctx.sub_state, 1);
    ASSERT_TRUE(ctx.walking);
    ASSERT_FALSE(ctx.running);
    ASSERT_EQ(ctx.speed, 3.0f);
}

UTEST(HierarchicalFSM, SubStateUpdate) {
    HierContext ctx;
    using ParentFSM = mameFSM::FSM<HierContext, IdleState, MovingState>;
    ParentFSM::Runner<IdleState> parent_fsm(ctx);
    
    ctx.speed = 1.0f;
    parent_fsm.update(ctx);  // Transition to MovingState
    float initial_speed = ctx.speed;
    
    parent_fsm.update(ctx);  // Update MovingState which updates sub FSM
    
    ASSERT_GT(ctx.speed, initial_speed);
}

UTEST(HierarchicalFSM, SubStateTransition) {
    HierContext ctx;
    SubFSM::Runner<SubWalk> sub_fsm(ctx);
    
    ASSERT_EQ(ctx.sub_state, 1);
    ASSERT_TRUE(ctx.walking);
    ASSERT_EQ(ctx.speed, 3.0f);
    
    ctx.should_run = true;
    sub_fsm.update(ctx);  // Walk -> Run
    ctx.should_run = false;  // Reset flag after transition
    
    ASSERT_EQ(ctx.sub_state, 2);
    ASSERT_FALSE(ctx.walking);
    ASSERT_TRUE(ctx.running);
    ASSERT_EQ(ctx.speed, 10.0f);
    
    // Continue running until speed drops
    for (int i = 0; i < 15; ++i) {
        sub_fsm.update(ctx);
    }
    
    // Should be back to walking
    ASSERT_EQ(ctx.sub_state, 1);
    ASSERT_TRUE(ctx.walking);
    ASSERT_FALSE(ctx.running);
}

UTEST(HierarchicalFSM, ParentTransitionCleansSubFSM) {
    HierContext ctx;
    using ParentFSM = mameFSM::FSM<HierContext, IdleState, MovingState>;
    ParentFSM::Runner<IdleState> parent_fsm(ctx);
    
    ctx.speed = 1.0f;
    parent_fsm.update(ctx);  // Idle -> Moving
    ASSERT_EQ(ctx.sub_state, 1);
    
    ctx.speed = -1.0f;  // Set negative to ensure transition to Idle
    parent_fsm.update(ctx);  // Moving -> Idle
    
    ASSERT_EQ(ctx.parent_state, 1);
    ASSERT_EQ(ctx.sub_state, 0);
    ASSERT_FALSE(ctx.walking);
    ASSERT_FALSE(ctx.running);
}