#pragma once

#include <vector>
#include <memory>
#include "card.h"

namespace aura
{

class deck
{
public:
    //! Returns null when empty
    virtual std::unique_ptr<card> draw() noexcept = 0;

    virtual size_t num_remaining() const noexcept;
};

class fixed_deck : public deck
{
public:
    fixed_deck(){}
    fixed_deck(std::vector<std::unique_ptr<card>> cards)
        : m_cards{std::move(cards)}
    {}

    //! Returns null when empty
    std::unique_ptr<card> draw() noexcept override
    {
        std::unique_ptr<card> out;
        m_cards.back().swap(out);
        m_cards.pop_back();
        return out;
    }

    virtual size_t num_remaining() const noexcept { return m_cards.size(); }
private:
    std::vector<std::unique_ptr<card>> m_cards;
};

}
