#pragma once

#include <cinder/Rect.h>
#include <string>
#include <chrono>
#include <functional>

namespace aura
{

struct anim_info
{
  //! total time elapsed since the beginning of the animation
  std::chrono::nanoseconds time_elapsed;

  //! ratio of time elapsed from the total specified duration
  float ratio_elapsed;

  //! 
  int render_index;
};

using anim_renderer_t = std::function<void(anim_info&)>;

struct dynamic_animation
{
  //! Duration of the animation after which it is to be removed
  std::chrono::nanoseconds total_duration;

  int index;

  std::chrono::steady_clock::time_point time_triggered = std::chrono::steady_clock::now();
  
  anim_renderer_t renderer;
};

inline auto make_animation(std::chrono::nanoseconds duration, anim_renderer_t renderer)
{
  return dynamic_animation{duration, 0, std::chrono::steady_clock::now(), std::move(renderer)};
}

} // namespace aura


