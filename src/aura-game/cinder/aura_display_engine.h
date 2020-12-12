#include <cinder/app/App.h>
#include <cinder/Area.h>
#include "shader.h"
#include <aura/session.h>

namespace aura
{

enum class reaction_t
{
    success,
    unsubscribe // tells caller to not send this message again
};

class visual_info
{
public:
    virtual ci::gl::Texture2dRef request_texture(std::string const& name) = 0;

    virtual ci::gl::BatchRef request_batch(std::string const& name) = 0;

    virtual phong_shader& get_phong_shader() noexcept = 0;
    virtual plain_shader& get_plain_shader() noexcept = 0;

    virtual ci::Area window_bounds() const noexcept = 0;

    virtual ci::vec3 window_to_world(ci::vec2 const& v) const noexcept = 0;

    virtual ~visual_info() = default;
};

class session;

class aura_display_engine
  : public ci::app::App
  , public visual_info
{
public:
    void setup() override;

    void draw() override;

    //void mouseDown(ci::app::MouseEvent ev) override;
    void on_session_notify(action_info const&);

public:
    ci::gl::Texture2dRef request_texture(std::string const& name) override;
    ci::gl::BatchRef request_batch(std::string const& name) override;

    phong_shader& get_phong_shader() noexcept override { return m_shader; }
    plain_shader& get_plain_shader() noexcept override { return m_plain_shader; }

    ci::Area window_bounds() const noexcept { return getWindowBounds(); }

    ci::vec3 window_to_world(ci::vec2 const& v) const noexcept override
    {
        return ci::gl::windowToWorldCoord(v);
    }

private:
    ci::CameraPersp m_cam;
    ci::CameraOrtho m_ortho;

    phong_shader m_shader;
    plain_shader m_plain_shader;

    std::unordered_map<std::string, ci::gl::Texture2dRef> m_textures;
    std::unordered_map<std::string, ci::gl::BatchRef> m_batches;

    aura_menu m_menu;

    std::unique_ptr<session> m_session;

    float eye_z = 15.0f;
};

} // namespace aura

