#include "mameFSM.hpp"
#include "utest.h"

#include <string>

struct ForceContext {
    int value = 0;
    std::string last_state;
};

struct NormalState;
struct ProtectedState;
struct EmergencyStateBase;
using EmergencyState = mameFSM::ForceTransitionable<EmergencyStateBase>;

// Define transition variant
using ForceTransition =
    std::variant<mameFSM::Transition<NormalState>, mameFSM::Transition<ProtectedState>,
                 mameFSM::Transition<EmergencyState>>;

struct NormalState {
    void on_enter(ForceContext& ctx) {
        ctx.last_state = "Normal";
        ctx.value = 100;
    }

    void on_exit(ForceContext&) {}

    auto on_update(ForceContext& ctx) -> ForceTransition {
        ctx.value++;
        return mameFSM::make_transition<NormalState>();
    }
};

struct ProtectedState {
    void on_enter(ForceContext& ctx) {
        ctx.last_state = "Protected";
        ctx.value = 200;
    }

    void on_exit(ForceContext&) {}

    auto on_update(ForceContext& ctx) -> ForceTransition {
        ctx.value += 2;
        return mameFSM::make_transition<ProtectedState>();
    }
};

struct EmergencyStateBase {
    void on_enter(ForceContext& ctx) {
        ctx.last_state = "Emergency";
        ctx.value = 999;
    }

    void on_exit(ForceContext&) {}

    auto on_update(ForceContext& ctx) -> ForceTransition {
        ctx.value = 0;
        return mameFSM::make_transition<NormalState>();
    }
};

UTEST(ForceTransition, ForceTransitionableDecorator) {
    ForceContext ctx;
    using TestFSM = mameFSM::FSM<ForceContext, NormalState, ProtectedState, EmergencyState>;
    TestFSM::Runner<NormalState> fsm(ctx);

    ASSERT_STREQ(ctx.last_state.c_str(), "Normal");
    ASSERT_EQ(ctx.value, 100);
}

UTEST(ForceTransition, ForceTransitionToEmergency) {
    ForceContext ctx;
    using TestFSM = mameFSM::FSM<ForceContext, NormalState, ProtectedState, EmergencyState>;
    TestFSM::Runner<ProtectedState> fsm(ctx);

    ASSERT_STREQ(ctx.last_state.c_str(), "Protected");
    ASSERT_EQ(ctx.value, 200);

    fsm.force_transition<EmergencyState>(ctx);
    ASSERT_STREQ(ctx.last_state.c_str(), "Emergency");
    ASSERT_EQ(ctx.value, 999);
    ASSERT_TRUE(fsm.is_in_state<EmergencyState>());
}

UTEST(ForceTransition, UpdateAfterForceTransition) {
    ForceContext ctx;
    using TestFSM = mameFSM::FSM<ForceContext, NormalState, ProtectedState, EmergencyState>;
    TestFSM::Runner<NormalState> fsm(ctx);

    fsm.force_transition<EmergencyState>(ctx);
    ASSERT_EQ(ctx.value, 999);

    fsm.update(ctx); // Should transition to NormalState
    ASSERT_EQ(ctx.value, 100);
    ASSERT_TRUE(fsm.is_in_state<NormalState>());
}

UTEST(ForceTransition, TypeTraits) {
    // Test that EmergencyState satisfies ForceTransitionableState concept
    ASSERT_TRUE(mameFSM::ForceTransitionableState<EmergencyState>);
    ASSERT_FALSE(mameFSM::ForceTransitionableState<NormalState>);
    ASSERT_FALSE(mameFSM::ForceTransitionableState<ProtectedState>);

    ASSERT_TRUE(EmergencyState::force_transition_allowed);
}