#pragma once

#include <any>
#include <vector>

#include <cinder/Color.h>
#include <cinder/gl/gl.h>

namespace aura
{

class phong_shader
{
public:
    virtual void setup() noexcept;

    //! sets light position in 'world' coordinates
    virtual void set_light_position(ci::vec3 const&) noexcept;

    virtual void set_ambient_light(ci::ColorAf col) noexcept;

    //! texture may not be null
    //! normal_map may be null
    virtual void draw(ci::gl::Texture2dRef texture,
                      ci::gl::Texture2dRef normal_map,
                      std::vector<ci::gl::BatchRef> targets) noexcept;

    virtual ci::gl::GlslProgRef program() const noexcept
    {
        return m_glsl;
    }

private:
    ci::gl::GlslProgRef m_glsl = nullptr;
};

class plain_shader
{
public:
    virtual void setup() noexcept;

    //! texture may not be null
    //! normal_map may be null
    virtual void draw(ci::ColorAf const& color,
                      std::vector<ci::gl::BatchRef> targets) noexcept;

    virtual ci::gl::GlslProgRef program() const noexcept
    {
        return m_glsl;
    }

private:
    ci::gl::GlslProgRef m_glsl = nullptr;
};

} // namespace aura