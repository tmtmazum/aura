#include "cind_display_engine.h"
#include <cinder/Cinder.h>
#include <cinder/app/RendererGl.h>

namespace {

template <typename T>
void enable_console(T* settings)
{
#ifdef CINDER_MSW_DESKTOP
  settings->setConsoleWindowEnabled(true);
#endif
}

} // namespace {}

CINDER_APP(aura::cind_display_engine, ci::app::RendererGl, [](auto* settings)
{
  enable_console(settings);
  //settings->setFullScreen(true);

  settings->setWindowSize(1024, 1024);
  settings->setResizable(true);

  //settings->setBorderless(true);
  settings->setAlwaysOnTop(true);
  settings->setTitle("Aura");
  settings->setFrameRate(60.0f);
})
