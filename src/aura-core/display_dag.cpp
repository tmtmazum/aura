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

void dag_node::set_child(shared_dag_t child) noexcept
{
  AURA_ASSERT(!m_child);

  child->m_replace_me = [w = weak_from_this()](shared_dag_t new_node)
  {
    if (auto const parent = w.lock())
    {
      std::unique_lock lk{parent->m_mutex};
      parent->m_child = std::move(new_node);
    }
  };

  std::unique_lock lk{m_mutex};
  m_child = std::move(child);
}

dag_id generate_dag_id()
{
  static std::atomic<uint64_t> i = 0;
  return i++;
}

void dag_node::set_sibling(shared_dag_t sibling) noexcept
{
  AURA_ASSERT(!m_sibling);

  sibling->m_replace_me = [w = weak_from_this()](shared_dag_t new_node)
  {
    if (auto const parent = w.lock())
    {
      std::unique_lock lk{parent->m_mutex};
      parent->m_sibling = std::move(new_node);
    }
  };

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

    std::shared_lock lock{d->m_mutex};

    auto const is_visited = visited.count(d.get());

    if (d && !is_visited)
    {
      for (auto&& [pred, next] : d->m_transitions)
      {
        AURA_ASSERT(pred);
        if (pred(data, *d))
        {
          AURA_ASSERT(d->m_replace_me);
          next->m_replace_me = std::move(d->m_replace_me);
          next->reset_timer();
          next->m_replace_me(next);
        }
        stack.push(next);
        continue;
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
