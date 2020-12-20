#include <cinder/app/App.h>
#include <cinder/Area.h>
#include <aura/session.h>
#include "shader.h"
#include "aura_menu.h"
#include "visual_info.h"
#include <aura/server_connection.h>

namespace aura
{

enum class reaction_t
{
    success,
    unsubscribe // tells caller to not send this message again
};

class session;

enum class primitive
{
    wire_plane,
    rect
};

class aura_display_engine : public ci::app::App, public visual_info
{
public:
    void setup() override;

    void draw() override;

    // void mouseDown(ci::app::MouseEvent ev) override;
    void on_session_notify(action_info const&);

public:
    aura_display_engine() = default;

    ci::gl::Texture2dRef request_texture(std::string const& name) override;
    ci::gl::BatchRef     request_batch(std::string const& name) override;

    phong_shader const& get_phong_shader() const noexcept override { return m_shader; }
    plain_shader const& get_plain_shader() const noexcept override
    {
        return m_plain_shader;
    }

    plain2d_shader const& get_plain2d_shader() const noexcept override { return m_plain2d_shader; }

    ci::Area window_bounds() const noexcept { return getWindowBounds(); }

    ci::vec3 window_to_world(ci::vec2 const& v) const noexcept override
    {
        return ci::gl::windowToWorldCoord(v);
    }

    bool is_online() const noexcept override
    {
        return m_connection.is_connected();
    }

private:
    ci::CameraPersp m_cam;
    ci::CameraOrtho m_ortho;

    phong_shader m_shader;
    plain_shader m_plain_shader;
    plain2d_shader m_plain2d_shader;

    std::unordered_map<std::string, ci::gl::Texture2dRef> m_textures;
    std::unordered_map<std::string, ci::gl::BatchRef>     m_batches;

    aura_menu m_menu;

    std::unique_ptr<session> m_session;

    server_connection m_connection;

    float eye_z = 15.0f;
};

} // namespace aura

