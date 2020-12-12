#pragma once

#include <filesystem>
#include <any>

namespace aura
{

class aura_controller
{
public:
    aura_controller(std::filesystem::path const& root_config);
};

} // namespace
