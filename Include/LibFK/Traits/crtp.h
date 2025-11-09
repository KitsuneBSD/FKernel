#pragma once

/**
 * @brief Curiously Recurring Template Pattern (CRTP) base class.
 *
 * Inherit from this class to enable CRTP for your derived classes.
 *
 * Example:
 * template <typename Derived>
 * class MyCRTPClass : public CRTP<MyCRTPClass<Derived>, Derived> {
 * public:
 *     void foo() {
 *         static_cast<Derived*>(this)->bar();
 *     }
 * };
 *
 * class ConcreteClass : public MyCRTPClass<ConcreteClass> {
 * public:
 *     void bar() {
 *         // Implementation
 *     }
 * };
 *
 * @tparam Base The base class in the CRTP hierarchy (e.g., MyCRTPClass<Derived>)
 * @tparam Derived The actual derived class
 */
template <typename Base, typename Derived>
struct CRTP {
    /**
     * @brief Get a reference to the derived object.
     * @return Reference to the derived object.
     */
    Derived& as_derived() { return static_cast<Derived&>(*this); }

    /**
     * @brief Get a const reference to the derived object.
     * @return Const reference to the derived object.
     */
    const Derived& as_derived() const { return static_cast<const Derived&>(*this); }
};
