#pragma once

#include <memory>
#include <functional>
#include <string>

namespace aura
{
class aura_menu
{
    using op_t = std::function<void(void)>;
    struct menu_item
    {
        std::string                             m_name;
        op_t                                    m_op;
        std::shared_ptr<menu_item>              m_parent;
        std::vector<std::shared_ptr<menu_item>> m_children;
    };

public:
    std::shared_ptr<menu_item> add_item(std::shared_ptr<menu_item> parent, std::string item_name, op_t op)
    {
        auto const item = std::make_shared<menu_item>(item_name, op, parent ? parent : m_head, {});
        auto p = parent ? parent : m_head;
        p.m_children.emplace_back(item);
        return item;
    }

    bool is_hidden() const noexcept { return m_hidden; }

    void show() noexcept { m_hidden = false; }

    void hide() noexcept { m_hidden = true; }

    void draw() const noexcept;

private:
    std::shared_ptr<menu_item> m_head = std::make_shared<menu_item>();

    bool m_hidden = false;
};

} // namespace aura
