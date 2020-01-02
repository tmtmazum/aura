#pragma once

#include <functional>
#include <algorithm>
#include <numeric>
#include <cinder/Rect.h>

namespace aura
{

enum class horizontal_alignment_t
{
  left,
  right,
  center
};

enum class vertical_alignment_t
{
  top,
  bottom,
  center
};


//! Convenience class for displaying grid-based items
struct grid
{
  ci::Rectf bounds;
  float padding_x{0.0f};
  float padding_y{0.0f};
  float element_width{0.0f};
  float element_height{0.0f};
  horizontal_alignment_t  horizontal_alignment = horizontal_alignment_t::left;
  vertical_alignment_t    vertical_alignment = vertical_alignment_t::top;

  void set_padding(float x, float y)
  {
    padding_x = x;
    padding_y = y;
  }

  void set_element_size(float x, float y)
  {
    element_width = x;
    element_height = y;
  }

  void align_horizontal(horizontal_alignment_t ha)
  {
    horizontal_alignment = ha;
  }

  void align_vertical(vertical_alignment_t va)
  {
    vertical_alignment = va;
  }

  std::pair<float, float> measure(int num_horizontal, int num_vertical)
  {
    return std::pair{num_horizontal*(element_width + padding_x*2), num_vertical*(element_height + padding_y*2)};
  }

  template <typename RectConsumer>
  void arrange(int num_horizontal, int num_vertical, RectConsumer const& cn) const
  {
    AURA_ASSERT(num_horizontal > 0);
    AURA_ASSERT(num_vertical > 0);
    auto cur_y = [&]()
    {
      auto const vert_space_needed = (element_height + (padding_y*2)) * num_vertical; 
      switch (vertical_alignment)
      {
      case vertical_alignment_t::top: return bounds.y1;
      case vertical_alignment_t::center: return bounds.y1 + ((bounds.getHeight() - vert_space_needed) / 2);
      case vertical_alignment_t::bottom: return bounds.y1 + (bounds.getHeight() - vert_space_needed);
      }
      return bounds.y1;
    }();

    auto cur_x = [&]()
    {
      auto const hor_space_needed = (element_width + (padding_x*2)) * num_horizontal;
      switch (horizontal_alignment)
      {
      case horizontal_alignment_t::left: return bounds.x1;
      case horizontal_alignment_t::center: return bounds.x1 + ((bounds.getWidth() - hor_space_needed) / 2);
      case horizontal_alignment_t::right: return bounds.x1 + (bounds.getWidth() - hor_space_needed);
      }
      return bounds.x1;
    }();

    auto const starting_x = cur_x;

    for (int i = 0; i < num_vertical; ++i)
    {
      cur_y += padding_y;
      cur_x = starting_x;
      for (int j = 0; j < num_horizontal; ++j)
      {
        cur_x += padding_x;
        auto [x1, x2] = std::pair{cur_x, cur_x + element_width};
        auto [y1, y2] = std::pair{cur_y, cur_y + element_height};
        cn(ci::Rectf{x1, y1, x2, y2});
        cur_x += element_width + padding_x;
      }
      cur_y += element_height + padding_y;
    }
  }

  template <typename Container, typename Consumer>
  void arrange_horizontally(Container&& c, Consumer const& cons)
  {
    using std::begin;
    using std::end;

    auto it = begin(c);
    arrange(c.size(), 1, [&](auto const& rect)
    {
      cons(*it++, rect);
    });
  }

  template <typename Container, typename Consumer>
  void arrange_vertically(Container&& c, Consumer const& cons)
  {
    using std::begin;
    using std::end;

    auto it = begin(c);
    arrange(1, c.size(), [&](auto const& rect)
    {
      cons(*it++, rect);
    });
  }
};

auto make_grid(ci::Rectf const& b)
{
  grid g{};
  g.bounds = b;
  g.bounds.canonicalize();
  return g;
}

struct frame
{
  ci::Rectf bounds;
  float min_padding_x{0.0f};
  float min_padding_y{0.0f};
  float max_padding_x{999999.0f};
  float max_padding_y{999999.0f};
  bool stretch{true};

  horizontal_alignment_t  horizontal_alignment = horizontal_alignment_t::left;
  vertical_alignment_t    vertical_alignment = vertical_alignment_t::top;

  using eval_t = std::function<void(ci::Rectf const&)>;

  struct element_info
  {
    float x;
    float y;
    eval_t eval;
  };

  std::vector<element_info> elements;

  void align_horizontal(horizontal_alignment_t ha)
  {
    horizontal_alignment = ha;
  }

  void align_vertical(vertical_alignment_t va)
  {
    vertical_alignment = va;
  }

  void set_min_padding(float x, float y)
  {
    min_padding_x = x;
    min_padding_y = y;
  }

  void set_max_padding(float x, float y)
  {
    max_padding_x = x;
    max_padding_y = y;
  }

  void add(float x, float y, eval_t ev)
  {
    elements.emplace_back(element_info{x, y, std::move(ev)});
  }

  void set_stretch(bool b) { stretch = b; }

  template <typename RectConsumer>
  void arrange_horizontally(RectConsumer const& cons)
  {
    auto const total_width_required = 
      std::accumulate(begin(elements), end(elements), 0,
        [](int val, auto const& el) { return val + el.width; });
  }

  float aligned_x(float element_width, float padding_x)
  {
      auto const hor_space_needed = (element_width + (padding_x*2));
      switch (horizontal_alignment)
      {
      case horizontal_alignment_t::left: return bounds.x1;
      case horizontal_alignment_t::center: return bounds.x1 + ((bounds.getWidth() - hor_space_needed) / 2);
      case horizontal_alignment_t::right: return bounds.x1 + (bounds.getWidth() - hor_space_needed);
      }
      return bounds.x1;
  }

  float aligned_y(float element_height, float padding_y)
  {
    auto const vert_space_needed = (element_height + (padding_y*2)); 
    switch (vertical_alignment)
    {
    case vertical_alignment_t::top: return bounds.y1;
    case vertical_alignment_t::center: return bounds.y1 + ((bounds.getHeight() - vert_space_needed) / 2);
    case vertical_alignment_t::bottom: return bounds.y1 + (bounds.getHeight() - vert_space_needed);
    }
    return bounds.y1;
  }

  void arrange_horizontally()
  {
    auto const total_width_required = 
      std::accumulate(begin(elements), end(elements), 0.0f,
        [](float val, auto const& el) { return val + el.x; });

    auto const width_avail = bounds.getWidth() - total_width_required;

    auto const pad_x = stretch ? std::clamp(width_avail / (elements.size() * 2), min_padding_x, max_padding_x) : min_padding_x;
    auto const pad_y = min_padding_y;

    auto cur_x = bounds.x1;
    for (auto const& el : elements)
    {
      auto const cur_y = aligned_y(el.y, min_padding_y) + pad_y;
      cur_x += pad_x;

      auto [x1, x2] = std::minmax(cur_x, cur_x + el.x);
      auto [y1, y2] = std::minmax(cur_y, cur_y + el.y);

      ci::Rectf r{x1, y1, x2, y2};
      el.eval(r);

      cur_x += r.getWidth();
      cur_x += pad_x;
    }
  }

  void arrange_vertically()
  {
    auto const total_height_required = 
      std::accumulate(begin(elements), end(elements), 0.0f,
        [](float val, auto const& el) { return val + el.y; });

    auto const height_avail = bounds.getHeight() - total_height_required;

    auto const pad_x = min_padding_x;
    auto const pad_y = stretch ? std::clamp(height_avail / (elements.size() * 2), min_padding_y, max_padding_y) : min_padding_y;

    auto cur_y = bounds.y1;
    for (auto const& el : elements)
    {
      auto const cur_x = aligned_x(el.x, min_padding_x) + pad_x;
      cur_y += pad_y;

      auto [x1, x2] = std::minmax(cur_x, cur_x + el.x);
      auto [y1, y2] = std::minmax(cur_y, cur_y + el.y);

      ci::Rectf r{x1, y1, x2, y2};
      el.eval(r);

      cur_y += r.getHeight();
      cur_y += pad_y;
    }
  }
};

auto make_frame(ci::Rectf const& r)
{
  frame f{};
  f.bounds = r;
  f.bounds.canonicalize();
  return f;
}

} // namespace aura
