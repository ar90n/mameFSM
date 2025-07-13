#pragma once

#include <concepts>
#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <variant>

namespace mameFSM {

template <typename T, typename... Ts>
constexpr bool contains_v = (std::same_as<T, Ts> || ...);

template <typename Base>
struct ForceTransitionable : Base {
    using Base::Base;
    static constexpr bool force_transition_allowed = true;
};

template <typename T>
concept ForceTransitionableState = requires {
    { T::force_transition_allowed } -> std::convertible_to<const bool&>;
} && T::force_transition_allowed;

template <typename Next, typename... Args>
struct Transition {
    using next = Next;
    using tuple = std::tuple<Args...>;
    std::tuple<Args...> args;
    explicit Transition(Args&&... a) : args{std::forward<Args>(a)...} {}
};

template <typename Next, typename... Args>
constexpr auto make_transition(Args&&... a) {
    return Transition<Next, std::decay_t<Args>...>{std::forward<Args>(a)...};
}

template <typename T>
struct is_transition_type : std::false_type {};

template <typename N, typename... A>
struct is_transition_type<Transition<N, A...>> : std::true_type {};

template <typename T>
struct is_variant_of_transitions : std::false_type {};

template <typename... Ts>
struct is_variant_of_transitions<std::variant<Ts...>>
    : std::bool_constant<(is_transition_type<Ts>::value && ...)> {};

template <typename T>
concept TransitionType =
    is_transition_type<std::decay_t<T>>::value || is_variant_of_transitions<std::decay_t<T>>::value;

template <typename S, typename Ctx>
concept State = requires(S s, Ctx& ctx) {
    { s.on_enter(ctx) } -> std::same_as<void>;
    { s.on_exit(ctx) } -> std::same_as<void>;
    { s.on_update(ctx) } -> TransitionType;
};

template <typename Context, typename... States>
    requires(State<States, Context> && ...)
struct FSM {
    using FirstState = std::tuple_element_t<0, std::tuple<States...>>;

    template <typename InitialState = FirstState>
        requires contains_v<InitialState, States...>
    class Runner {
        using StateVariant = std::variant<std::monostate, States...>;
        StateVariant current_;

        template <typename Target, typename... Args>
            requires contains_v<Target, States...>
        void switch_to(Context& ctx, Args&&... a) {
            std::visit(
                [&]<typename Cur>(Cur& s) {
                    if constexpr (!std::same_as<Cur, std::monostate>)
                        s.on_exit(ctx);
                },
                current_);

            current_.template emplace<Target>(std::forward<Args>(a)...);
            std::get<Target>(current_).on_enter(ctx);
        }

        template <typename Cur, typename TrType>
        void apply_transition(Context& ctx, TrType&& tr) {
            using ActualTr = std::decay_t<TrType>;
            using Next = typename ActualTr::next;
            static_assert(contains_v<Next, States...>, "Transition target not in FSM states");

            if constexpr (!std::same_as<Next, Cur>) {
                std::apply(
                    [&](auto&&... a) { switch_to<Next>(ctx, std::forward<decltype(a)>(a)...); },
                    tr.args);
            }
        }

      public:
        explicit Runner(Context& ctx) {
            switch_to<InitialState>(ctx);
        }

        void update(Context& ctx) {
            std::visit(
                [&]<typename Cur>(Cur& cur_state) {
                    if constexpr (std::same_as<Cur, std::monostate>) {
                        return;
                    } else {
                        auto tr = cur_state.on_update(ctx);
                        using Tr = std::decay_t<decltype(tr)>;

                        // Handle both single transition and variant of transitions
                        if constexpr (is_transition_type<Tr>::value) {
                            apply_transition<Cur>(ctx, tr);
                        } else if constexpr (is_variant_of_transitions<Tr>::value) {
                            std::visit(
                                [&]<typename ActualTr>(ActualTr&& actual_tr) {
                                    apply_transition<Cur>(ctx, std::forward<ActualTr>(actual_tr));
                                },
                                tr);
                        }
                    }
                },
                current_);
        }

        template <typename Target>
            requires ForceTransitionableState<Target> && contains_v<Target, States...>
        void force_transition(Context& ctx) {
            switch_to<Target>(ctx);
        }

        template <typename T>
        [[nodiscard]] bool is_in_state() const {
            return std::holds_alternative<T>(current_);
        }
    };
};

} // namespace mameFSM