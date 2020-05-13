#include "display_dag.h"
#include <unordered_set>
#include <stack>
#include <shared_mutex>

namespace aura
{

namespace
{

void render(dag_node* d, std::any const& data)
{
  if (d->is_selected() && d->m_on_render_selected)
  {
    d->m_on_render_selected(data);
  }
  else if (d->is_hovered() && d->m_on_render_hovered)
  {
    d->m_on_render_hovered(data);
  }
  else if(d->m_on_render)
  {
    d->m_on_render(data);
  }
}

} // namespace {}

void master_dag::add_layer(std::shared_ptr<dag_node>&& new_layer) noexcept
{
  std::lock_guard lock{m_top_layer->m_mutex};
  m_top_layer->m_sibling = std::move(new_layer);
  m_top_layer = m_top_layer->m_sibling;
}

void master_dag::render_all(std::any const& data)
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
      if (d->m_on_push)
      {
        d->m_on_push(data);
      }
      render(d.get(), data);
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
