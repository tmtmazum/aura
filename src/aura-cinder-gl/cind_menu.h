#pragma once

#include <aura-core/display_dag.h>
#include <functional>
#include <memory>
#include "cind_menu_style.h"

namespace aura
{
class cind_menu
{
   public:
    using action_internal_t = std::function<void(void*)>;

    cind_menu(cind_menu_style style)
        : m_style{std::make_shared<cind_menu_style const>(style)}
    {
        setup_form();
    }

    template <typename DisplayContext, typename Fn>
    void add_button(std::string text, Fn action)
    {
        add_button_internal(std::move(text),
                            [f = std::move(action)](auto const voidstar) {
                                f(reinterpret_cast<DisplayContext*>(voidstar));
                            });
    }

    template <typename DisplayContext, typename Fn>
    void add_close_button(Fn action)
    {
        add_close_button_internal([f = std::move(action)](auto const voidstar) {
            f(reinterpret_cast<DisplayContext*>(voidstar));
        });
    }

    //! Execute the current selected button
    void execute_current_button();

    void execute_close_button();

    void hide() noexcept;

    void show() noexcept;

    bool is_hidden() const noexcept;

    auto get_node() const noexcept { return m_node; }

   private:
    void setup_form();

    void add_button_internal(std::string text, action_internal_t action);

    void add_close_button_internal(action_internal_t action);

   private:
    shared_dag_t m_node = std::make_shared<dag_node>(dag_node_s{});

    int m_num_buttons = 0;

    std::shared_ptr<cind_menu_style const> m_style;
};

}    // namespace aura
