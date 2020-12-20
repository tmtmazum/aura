#pragma once

#include <cinder/gl/gl.h>
#include "shader.h"

namespace aura
{

class visual_info
{
public:
    virtual ci::gl::Texture2dRef request_texture(std::string const& name) = 0;

    virtual ci::gl::BatchRef request_batch(std::string const& name) = 0;

    virtual phong_shader const& get_phong_shader() const noexcept = 0;
    virtual plain_shader const& get_plain_shader() const noexcept = 0;
    virtual plain2d_shader const& get_plain2d_shader() const noexcept = 0;

    virtual ci::Area window_bounds() const noexcept = 0;

    virtual ci::vec3 window_to_world(ci::vec2 const& v) const noexcept = 0;

    virtual bool is_online() const noexcept = 0;

    virtual ~visual_info() = default;
};

} // namespace aura
