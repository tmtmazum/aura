#pragma once

#include <aura-core/build.h>
#include <unordered_map>
#include <memory>
#include <functional>
#include <shared_mutex>
#include <chrono>
#include <any>

namespace aura
{

class dag_node;
using dag_id = uint64_t;

//! Think of this as the abstract dag context that gets passed in for each render/push operation
using dag_context_t = void*;

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
  std::shared_ptr<dag_node> top_layer() const noexcept { return m_top_layer; }

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

  void render_all(dag_context_t data);

private:
  std::unordered_map<dag_id, std::weak_ptr<dag_node>> m_refs;
  std::shared_ptr<dag_node> m_master = std::make_shared<dag_node>();
  std::shared_ptr<dag_node> m_top_layer = m_master;
};

using shared_dag_t = std::shared_ptr<dag_node>;
using weak_dag_t = std::weak_ptr<dag_node>;

dag_id generate_dag_id();

class dag_node;

struct dag_node_s
{
  using render_fn_t = std::function<void(dag_context_t const&, dag_node&)>;
  using void_fn_t = std::function<void(void)>;
  using predicate_fn_t = std::function<bool(dag_context_t const&, dag_node&)>;
  using unique_predicate_fn_t = std::shared_ptr<predicate_fn_t>;
  using replace_me_t = std::function<void(shared_dag_t)>;
  using transition_map_t = std::vector<std::pair<predicate_fn_t, shared_dag_t>>;
  using clock_t = std::chrono::steady_clock;

  shared_dag_t      m_child;
  shared_dag_t      m_sibling;
  weak_dag_t        m_parent;
  transition_map_t  m_transitions;

  render_fn_t       m_on_push;
  render_fn_t       m_on_render;
  void_fn_t         m_on_pop;
  std::any          m_any_data;    //!< store some per node data

  dag_id m_id = generate_dag_id();
  clock_t::time_point m_timer_start = dag_node_s::clock_t::now();
};

enum class transition_function
{
  linear,
  ease_in_quadratic,
  ease_out_quadratic,
  ease_in_out_quadratic,
  ease_in_cubic,
  ease_out_cubic,
  ease_in_out_cubic,
  ease_in_quartic,
  ease_out_quartic,
  ease_in_out_quartic,
  ease_in_quintic,
  ease_out_quintic,
  ease_in_out_quintic
};

class dag_node : public dag_node_s, public std::enable_shared_from_this<dag_node>
{
  friend class master_node;
public:
  dag_node() = default;
  explicit dag_node(dag_node_s impl)
    : dag_node_s{std::move(impl)}
  {
  }

  void add_child(shared_dag_t child) noexcept;

  void add_sibling(shared_dag_t sibling) noexcept;

  void set_child(shared_dag_t child) noexcept;
  
  void set_sibling(shared_dag_t sibling) noexcept;

  void reset_timer() noexcept { m_timer_start = dag_node_s::clock_t::now(); }

  auto time_elapsed() const noexcept
  {
    return dag_node_s::clock_t::now() - m_timer_start;
  }

  void clear() noexcept;

  template <typename T, typename Fn>
  void set_render_fn(Fn const& fn) noexcept
  {
    m_on_render = [f = std::move(fn)](auto const& any_val, auto& this_ref)
    {
      f(reinterpret_cast<T>(any_val), this_ref);
    };
  }

  template <typename T, typename Fn>
  void set_push_fn(Fn const& fn) noexcept
  {
    m_on_push = [f = std::move(fn)](auto const& any_val, auto& this_ref)
    {
      f(reinterpret_cast<T>(any_val), this_ref);
    };
  }

  //! Linear interval elapsed
  float interval_elapsed(dag_node_s::clock_t::duration total_duration) const noexcept;

  float transition_elapsed(dag_node_s::clock_t::duration total_duration, transition_function fn = transition_function::ease_out_cubic) noexcept;

  template <typename T, typename Fn>
  void add_transition(Fn&& fn, shared_dag_t new_node) noexcept
  {
    m_transitions.emplace_back([f = std::move(fn)](auto const& any_val, auto& this_ref)
    {
      return f(reinterpret_cast<T>(any_val), this_ref);
    }, std::move(new_node));
  }

  template <typename T>
  void store(T const& t)
  {
    m_any_data = std::any{t};
  }

  template <typename T>
  bool can_load()
  try
  {
    auto a = std::any_cast<T>(m_any_data);
    return true;
  }
  catch (std::bad_any_cast const& )
  {
    return false;
  }

  template <typename T>
  T load()
  {
    AURA_ASSERT(can_load<T>());
    return std::any_cast<T>(m_any_data);
  }

  std::shared_mutex m_mutex;
};

/* Types of transitions
 * 1. Timed transition (using t=0..1)
 * 2. Highlight transition (change render function)
 * 3. Select transition (change render function)
 * 4. Expire transition
 */

} // namespace aura
