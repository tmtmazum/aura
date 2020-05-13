#include "aura-core/build.h"
#include "main_functions.h"
#include <filesystem>
#include <cinder/app/AppBase.h>

namespace aura
{

//! setup assets from current directory
void setup_assets()
{
  auto const abs_path =
      std::filesystem::absolute(std::filesystem::current_path() / L"assets")
          .wstring();
  AURA_LOG(L"assets path = %ls", abs_path.c_str());
  ci::app::addAssetDirectory(abs_path);
}

} // namespace aura
