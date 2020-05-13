#pragma once

#include <unordered_map>
#include <memory>
#include <any>
#include <functional>
#include <mutex>

namespace aura
{

class dag_node;
using dag_id = uint64_t;

class master_dag
{
public:
  std::shared_ptr<dag_node> operator[](dag_id id) const noexcept;

  std::shared_ptr<dag_node> get_hovered() const noexcept;

  std::vector<std::shared_ptr<dag_node>> get_selected() const noexcept;

  //! Return the node for the layer that is visually at the top
  //! but at the bottom of the DAG
  //! May return null if no layer exists yet.
  //! E.g. :-
  //! m_master
  //! |
  //! V
  //! Layer 1 -> Layer 2 -> Layer 3 -> Layer 4 (Top)
  //! |            |          |          |
  //! V            V          V          V
  //! primitives..
  std::shared_ptr<dag_node> top_layer() const noexcept;

  //! adds a layer at the end / top
  void add_layer(std::shared_ptr<dag_node>&& new_layer) noexcept;

  //! Rendering the graph involves doing a pre-order DFS from the master node
  //! E.g. :-
  //! m_master
  //! |
  //! V
  //! Layer 1 -> Layer 2 -> Layer 3 -> Layer 4 (Top)
  //! |            |          |          |
  //! V            V          V          V
  //! A -> F -> G                        I
  //! |         |
  //! V         V
  //! B -> E    H
  //! |
  //! V
  //! C -> D
  //! The above graph would be rendered in the following order:
  //! A, B, C, E, F, G, H, I

  void render_all(std::any const& data);

private:
  std::unordered_map<dag_id, std::weak_ptr<dag_node>> m_refs;
  std::shared_ptr<dag_node> m_master = std::make_shared<dag_node>();
  std::shared_ptr<dag_node> m_top_layer = m_master;
};

struct dag_node_s
{
  std::shared_ptr<dag_node> m_child;
  std::shared_ptr<dag_node> m_sibling;
  std::shared_ptr<dag_node> m_transition;

  using render_fn_t = std::function<void(std::any const&)>;

  render_fn_t m_on_render;
  render_fn_t m_on_render_hovered;
  render_fn_t m_on_render_selected;

  dag_id m_id;
};

class dag_node : public dag_node_s
{
  friend class master_node;
public:
  dag_node() = default;
  explicit dag_node(dag_node_s impl)
    : dag_node_s{std::move(impl)}
  {
  }

  //dag_id id() const noexcept;

  //void set_hover() noexcept;
  //void rm_hover() noexcept;

  //void select() noexcept;
  //void deselect() noexcept;

  bool is_selected() const noexcept { return false; }
  bool is_hovered() const noexcept { return false; }
  std::mutex m_mutex;
};

} // namespace aura
