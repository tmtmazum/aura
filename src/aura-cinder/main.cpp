#include "cind_display_engine.h"
#include <cinder/app/RendererGL.h>

CINDER_APP(aura::cind_display_engine, ci::app::RendererGl, [](auto* settings)
{
  settings->setConsoleWindowEnabled(true);
  //settings->setFullScreen(true);

  settings->setWindowSize(1024, 1024);
  settings->setResizable(true);

  //settings->setBorderless(true);
  settings->setAlwaysOnTop(true);
  settings->setTitle("Aura");
  settings->setFrameRate(60.0f);
})