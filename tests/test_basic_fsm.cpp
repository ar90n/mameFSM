#include "utest.h"
#include "mameFSM.hpp"

struct TestContext {
    int value = 0;
    bool enter_called = false;
    bool exit_called = false;
    bool update_called = false;
    bool should_transition = false;
};

struct StateA;
struct StateB;

// Define transition variant for test FSM
using TestTransition = std::variant<
    mameFSM::Transition<StateA>,
    mameFSM::Transition<StateB>
>;

struct StateA {
    void on_enter(TestContext& ctx) {
        ctx.enter_called = true;
        ctx.value = 1;
    }
    
    void on_exit(TestContext& ctx) {
        ctx.exit_called = true;
    }
    
    auto on_update(TestContext& ctx) -> TestTransition {
        ctx.update_called = true;
        ctx.value++;
        if (ctx.should_transition) {
            return mameFSM::make_transition<StateB>();
        }
        return mameFSM::make_transition<StateA>();
    }
};

struct StateB {
    void on_enter(TestContext& ctx) {
        ctx.enter_called = true;
        ctx.value = 10;
    }
    
    void on_exit(TestContext& ctx) {
        ctx.exit_called = true;
    }
    
    auto on_update(TestContext& ctx) -> TestTransition {
        ctx.update_called = true;
        ctx.value += 10;
        if (ctx.should_transition) {
            return mameFSM::make_transition<StateA>();
        }
        return mameFSM::make_transition<StateB>();
    }
};

UTEST(BasicFSM, InitialState) {
    TestContext ctx;
    using TestFSM = mameFSM::FSM<TestContext, StateA, StateB>;
    TestFSM::Runner<StateA> fsm(ctx);
    
    ASSERT_TRUE(ctx.enter_called);
    ASSERT_EQ(ctx.value, 1);
    ASSERT_TRUE(fsm.is_in_state<StateA>());
    ASSERT_FALSE(fsm.is_in_state<StateB>());
}

UTEST(BasicFSM, StateUpdate) {
    TestContext ctx;
    using TestFSM = mameFSM::FSM<TestContext, StateA, StateB>;
    TestFSM::Runner<StateA> fsm(ctx);
    
    ctx.update_called = false;
    fsm.update(ctx);
    
    ASSERT_TRUE(ctx.update_called);
    ASSERT_EQ(ctx.value, 2);
}

UTEST(BasicFSM, StateTransition) {
    TestContext ctx;
    using TestFSM = mameFSM::FSM<TestContext, StateA, StateB>;
    TestFSM::Runner<StateA> fsm(ctx);
    
    ctx.enter_called = false;
    ctx.exit_called = false;
    ctx.should_transition = true;
    
    fsm.update(ctx);
    
    ASSERT_TRUE(ctx.exit_called);
    ASSERT_TRUE(ctx.enter_called);
    ASSERT_EQ(ctx.value, 10);
    ASSERT_FALSE(fsm.is_in_state<StateA>());
    ASSERT_TRUE(fsm.is_in_state<StateB>());
}

UTEST(BasicFSM, MultipleTransitions) {
    TestContext ctx;
    using TestFSM = mameFSM::FSM<TestContext, StateA, StateB>;
    TestFSM::Runner<StateA> fsm(ctx);
    
    ctx.should_transition = true;
    fsm.update(ctx);  // A -> B
    ASSERT_EQ(ctx.value, 10);
    ASSERT_TRUE(fsm.is_in_state<StateB>());
    
    ctx.should_transition = false;
    fsm.update(ctx);  // B stays
    ASSERT_EQ(ctx.value, 20);
    
    ctx.should_transition = true;
    fsm.update(ctx);  // B -> A
    ASSERT_EQ(ctx.value, 1);
    ASSERT_TRUE(fsm.is_in_state<StateA>());
    
    ctx.should_transition = false;
    fsm.update(ctx);  // A stays
    ASSERT_EQ(ctx.value, 2);
}