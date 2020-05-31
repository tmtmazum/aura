#include "display_dag.h"
#include <unordered_set>
#include <stack>
#include <shared_mutex>
#include <atomic>
#include <algorithm>
#include <aura-core/build.h>

namespace aura
{

void master_dag::add_layer(std::shared_ptr<dag_node>&& new_layer) noexcept
{
  std::lock_guard lock{m_top_layer->m_mutex};
  m_top_layer->m_sibling = std::move(new_layer);
  m_top_layer = m_top_layer->m_sibling;
}

void dag_node::clear() noexcept
{
  std::unique_lock lk{m_mutex};
  m_child.reset();
  m_sibling.reset();
  m_transitions.clear();
}

void dag_node::add_child(shared_dag_t child) noexcept
{
    if (m_child)
    {
        return m_child->add_sibling(std::move(child));
    }
    return set_child(std::move(child));
}

void dag_node::set_child(shared_dag_t child) noexcept
{
  AURA_ASSERT(!m_child);

  child->m_parent = weak_from_this();

  std::unique_lock lk{m_mutex};
  m_child = std::move(child);
}

dag_id generate_dag_id()
{
  static std::atomic<uint64_t> i = 0;
  return i++;
}

void dag_node::add_sibling(shared_dag_t sibling) noexcept
{
    if (m_sibling)
    {
        return m_sibling->add_sibling(std::move(sibling));
    }
    return set_sibling(std::move(sibling));
}

void dag_node::set_sibling(shared_dag_t sibling) noexcept
{
  AURA_ASSERT(!m_sibling);

  sibling->m_parent = weak_from_this();

  std::unique_lock lk{m_mutex};
  m_sibling = std::move(sibling);
}

//! Linear interval elapsed
float dag_node::interval_elapsed(dag_node_s::clock_t::duration total_duration) const noexcept
{
  using namespace std::chrono;
  return static_cast<float>(duration_cast<nanoseconds>(time_elapsed()).count()) / duration_cast<nanoseconds>(total_duration).count();
}

float dag_node::transition_elapsed(dag_node_s::clock_t::duration total_duration, transition_function fn) noexcept
{
  auto const t = std::clamp(interval_elapsed(total_duration), 0.0f, 1.0f);
  switch (fn)
  {
  case transition_function::linear: return t;
  case transition_function::ease_in_quadratic: return t*t;
  case transition_function::ease_out_quadratic: return t*(2.0f - t);
  case transition_function::ease_in_out_quadratic: return (t < 0.5f ? 2.0f*t*t : -1.0 + (4.0f - 2.0f*t)*t);
  case transition_function::ease_in_cubic: return t*t*t;
  case transition_function::ease_out_cubic:
  {
    auto const r = (1.0f - t);
    return 1.0f - (r*r*r);
  };
  case transition_function::ease_in_out_cubic:
  {
    return t < 0.5f ? (4.0f*t*t*t) : (t-1.0f)*(2.0f*t-2.0f)*(2.0f*t-2.0f)+1;
  };
  case transition_function::ease_in_quartic: return t*t*t*t;
  case transition_function::ease_out_quartic:
  {
    auto const r = (1.0f - t);
    return 1.0f - (r*r*r*r);
  };
  case transition_function::ease_in_out_quartic:
  {
    auto const r = -2.0f*t + 2.0f;
    return t < 0.5f ? 8.0f*t*t*t*t : 1.0f - r*r*r*r / 2.0f;
  }
  case transition_function::ease_in_quintic: return t*t*t*t*t;
  case transition_function::ease_out_quintic:
  {
    auto const r = (1.0f - t);
    return 1.0f - (r*r*r*r*r);
  }
  case transition_function::ease_in_out_quintic:
  {
    auto const r = -2.0f * t + 2.0f;
    return t < 0.5f ? 16.0f*t*t*t*t*t : 1.0f - r*r*r*r*r / 2.0f;
  }
  default: return t;
  }
  return t;
}

static void transition_node(shared_dag_t const& current, shared_dag_t const& next)
{
    next->m_sibling = std::move(current->m_sibling);
    current->m_sibling = nullptr;
    next->reset_timer();
    if (auto const parent = current->m_parent.lock())
    {
        next->m_parent = parent;
        std::unique_lock lk{parent->m_mutex};
        if (parent->m_child.get() == current.get())
        {
            parent->m_child = next;
        }
        else
        {
            AURA_ASSERT(parent->m_sibling.get() == current.get());
            parent->m_sibling = next;
        }
    }

    if (next->m_sibling)
    {
        next->m_sibling->m_parent = next;
    }
}

void master_dag::render_all(dag_context_t data)
{
    constexpr auto est_num_nodes = 32;
    std::unordered_set<dag_node*> visited;
    visited.reserve(est_num_nodes);

    std::stack<std::shared_ptr<dag_node>> stack;
    stack.push(m_master);

    while (!stack.empty())
    {
        auto const d = stack.top();
        AURA_ASSERT(d);

        std::shared_lock lock{d->m_mutex};

        auto const is_visited = visited.count(d.get());

        if (!is_visited)
        {
            for (auto&& [pred, next] : d->m_transitions)
            {
                AURA_ASSERT(pred);
                if (!pred(data, *d))
                {
                    continue;
                }
                // condition for transition is satisfied
                AURA_ASSERT(!next->m_sibling);

                transition_node(d, next);

                stack.push(next);
            }

            if (d->m_on_push)
            {
                d->m_on_push(data, *d);
            }

            if (d->m_on_render)
            {
                d->m_on_render(data, *d);
            }
            visited.insert(d.get());
        }

        if (d->m_child && !visited.count(d->m_child.get()))
        {
            stack.push(d->m_child);
            continue;
        }

        if (d->m_on_pop)
        {
            d->m_on_pop();
        }
        stack.pop();

        if (d->m_sibling && !visited.count(d->m_sibling.get()))
        {
            stack.push(d->m_sibling);
            continue;
        }
    }
}

} // namespace aura
