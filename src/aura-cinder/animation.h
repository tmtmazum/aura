#pragma once

#include <cinder/Rect.h>
#include <string>
#include <chrono>

namespace aura
{

struct static_animation
{
  ci::Rectf rect;
  std::wstring sprite_base;
  std::chrono::steady_clock::duration duration;
  int num_frames;
  std::chrono::steady_clock::time_point time_triggered = std::chrono::steady_clock::now();
};

} // namespace aura


