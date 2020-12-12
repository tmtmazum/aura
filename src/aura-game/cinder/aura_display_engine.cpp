#include "aura_display_engine.h"
#include <cinder/gl/gl.h>
#include <cinder/app/RendererGl.h>
#include <cinder/easing.h>
#include <aura/session.h>

namespace aura
{

using namespace ci;

class visual_object
{
public:
    virtual reaction_t setup(visual_info&) noexcept { return reaction_t::unsubscribe; }

    virtual reaction_t draw(visual_info&) noexcept { return reaction_t::unsubscribe; }

    virtual Rectf bounding_box() const noexcept = 0;

    virtual reaction_t hover_on() noexcept { return reaction_t::unsubscribe; }

    virtual reaction_t hover_off() noexcept { return reaction_t::unsubscribe; }

    virtual reaction_t mouse_left_on() noexcept { return reaction_t::unsubscribe; }

    virtual reaction_t mouse_left_off() noexcept { return reaction_t::unsubscribe; }

    //! signals caller to dispose of this object
    virtual bool is_finished() const noexcept { return false;}
};
    

class card_unit : public visual_object
{
    reaction_t setup(visual_info& info) noexcept override{
        return reaction_t::success;
    }

    Rectf bounding_box() const noexcept override
    {
        return {};
    }

    void set_position(vec2 v)
    {
    
    }
    
    reaction_t draw(visual_info& info) noexcept override
    {
    
    }
};

class card_draft_intro : public visual_object
{
public:
    reaction_t setup(visual_info& info) noexcept override{
        return reaction_t::success;
    }

    reaction_t draw(visual_info& info) noexcept override{
        auto w = 2*(0.7f);
        auto h = 2.0f;

        {
            ci::gl::ScopedModelMatrix mat{};
            info.get_plain_shader().draw(
                {1.0f, 0.0f, 0.0f},
                {info.request_batch("wire_plane")}
            );
        }

        {
            auto world_pos = vec3{-5.0f, 0.0f, 0.0f};

            ci::gl::ScopedModelMatrix mat{};
            ci::gl::translate(world_pos);
            ci::gl::scale({w, h, 1.0f});
            ci::gl::rotate(0.0f, {0.0f, 1.0f, 0.0f});

            info.get_phong_shader().draw(
                info.request_texture("card-hidden"),
                info.request_texture("normal-hidden"),
                {info.request_batch("rect")}
            );
        }

        static std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
        constexpr auto dur = std::chrono::milliseconds(2000);
        auto const interval_elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count()) / dur.count();

        for (int i = 0; i < 4; ++i)
        {
            constexpr auto start_x = -5.0f;
            auto const end_x = 3.0f - i*2.0f;
            auto const distance = easeOutQuad(std::min(interval_elapsed, 1.0f)) * (end_x - start_x);
            auto const rot_distance = easeOutQuad(std::min(interval_elapsed, 1.0f)) * (0.0f - (-M_PI));
                
            auto world_pos = vec3{start_x + distance, 0.0f, 0.0f};

            {
                ci::gl::ScopedModelMatrix mat{};
                ci::gl::translate(world_pos);
                ci::gl::scale({w, h, 1.0f});
                ci::gl::rotate(M_PI + rot_distance, {0.0f, 1.0f, 0.0f});

                info.get_phong_shader().draw(
                    info.request_texture("card-Potion of Speed"),
                    info.request_texture("normalmap-card"),
                    {info.request_batch("rect")}
                );
            }
            {
                ci::gl::ScopedModelMatrix mat{};
                world_pos.z += 0.1f;
                ci::gl::translate(world_pos);
                ci::gl::scale({w, h, 1.0f});
                ci::gl::rotate(rot_distance, {0.0f, 1.0f, 0.0f});

                info.get_phong_shader().draw(
                    info.request_texture("card-hidden"),
                    info.request_texture("normal-hidden"),
                    {info.request_batch("rect")}
                );
            }
        }
        return reaction_t::success;
    }

    Rectf bounding_box() const noexcept override
    {
        return {};
    }

    reaction_t hover_on() noexcept { return reaction_t::unsubscribe; }

    reaction_t hover_off() noexcept { return reaction_t::unsubscribe; }

    reaction_t mouse_left_on() noexcept { return reaction_t::unsubscribe; }

    reaction_t mouse_left_off() noexcept { return reaction_t::unsubscribe; }

    //! signals caller to dispose of this object
    bool is_finished() const noexcept { return false;}

private:
	gl::BatchRef m_batch;

};

void aura_display_engine::on_session_notify(action_info const& act_info)
{

}


void aura_display_engine::setup()
{
    setWindowSize({1024, 800});

    // start loading shaders and textures async

    m_menu.add_item(nullptr, "Local PvP", [&]()
    {
        m_session = make_local_pvp_session();
        m_session->register_notify(0, [this](auto const& act_info)
        {
            on_session_notify(act_info);
        });

        m_session->register_notify(1, [this](auto const& act_info)
        {
            on_session_notify(act_info);
        });
    });

    m_menu.add_item(nullptr, "Quit To Desktop", [&]()
    {
        action_info info{};
        info.act = action_type::forfeit;

        m_session->notify_action(info);
        m_session.reset();
    });

    /*
    m_shader.setup();
    m_plain_shader.setup();

	m_cam.lookAt({0.0f, -1.0f, 10.0f}, {0.0f, 0.0f, 0.0f});
    m_ortho.setOrtho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 10.0f);
    ci::gl::Texture2d::Format format{};
    format.loadTopDown();

    m_batches.emplace("rect", gl::Batch::create(ci::geom::Rect(), get_phong_shader().program()));

    auto wp = ci::geom::WirePlane();
    wp.subdivisions({10.0f, 10.0f});
    wp.axes({1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    wp.size({10.0f, 10.0f});
    m_batches.emplace("wire_plane", gl::Batch::create(wp, get_plain_shader().program()));

	gl::enableDepthWrite();
	gl::enableDepthRead();
	gl::enableAlphaBlending();
    gl::lineWidth(0.5f);
    gl::enableFaceCulling();
    gl::cullFace(GL_BACK);

    m_intro.setup(*this);
    */
}

void aura_display_engine::draw()
{
    gl::clear( ci::Color( 0.2f, 0.2f, 0.2f ) );

    if (m_menu.is_open())
    {
	m_menu.draw();
    }

    /*
	m_cam.lookAt({0.0f, -1.0f, eye_z}, {0.0f, 0.0f, 0.0f});

	gl::setMatrices(m_cam);
	//gl::setMatrices(m_ortho);

	auto mouse_world = ci::gl::windowToWorldCoord(getMousePos() - getWindowPos());
	mouse_world.z = 1.25f;

    m_shader.set_light_position(mouse_world);

    m_intro.draw(*this);
    */
}

ci::gl::Texture2dRef aura_display_engine::request_texture(std::string const& name)
{
    getWindowBounds();
    if (auto const it = m_textures.find(name); it != m_textures.end())
        return it->second;

    ci::gl::Texture2d::Format format{};
    format.loadTopDown();

    auto const name_full = name + ".png";

    auto const texture = ci::gl::Texture2d::create(ci::loadImage(loadAsset(name_full)), format);
    m_textures.emplace(name, texture);

    return texture;
}

ci::gl::BatchRef aura_display_engine::request_batch(std::string const& name)
{
    if (auto const it = m_batches.find(name); it != m_batches.end())
        return it->second;

    return nullptr;
}


} // namespace aura

namespace {

template <typename T>
void enable_console(T* settings)
{
#ifdef CINDER_MSW_DESKTOP
  settings->setConsoleWindowEnabled(true);
#endif
}

} // namespace {}

CINDER_APP(aura::aura_display_engine, ci::app::RendererGl, [](auto* settings)
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

