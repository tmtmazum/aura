#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include <cinder/Easing.h>

#include "phong_shader.h"
#include <filesystem>

using namespace ci;

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

    virtual vec3 window_to_world(vec2 const& v) const noexcept = 0;

    virtual ~visual_info() = default;
};

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

class TestApp3 : public ci::app::App, public visual_info
{
public:
    void setup() override;

    void draw() override;

	void mouseWheel(app::MouseEvent event)
    {
        if (event.isControlDown())
        {
            eye_z += event.getWheelIncrement() * 0.2f;
        }
    }
private:
    ci::gl::Texture2dRef request_texture(std::string const& name) override
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

    ci::gl::BatchRef request_batch(std::string const& name) override
    {
        if (auto const it = m_batches.find(name); it != m_batches.end())
            return it->second;

        return nullptr;
    }

    phong_shader& get_phong_shader() noexcept override { return m_shader; }
    plain_shader& get_plain_shader() noexcept override { return m_plain_shader; }

    ci::Area window_bounds() const noexcept { return getWindowBounds(); }

    vec3 window_to_world(vec2 const& v) const noexcept override
    {
        return ci::gl::windowToWorldCoord(v);
    }
private:
    ci::CameraPersp m_cam;
    ci::CameraOrtho m_ortho;

    phong_shader m_shader;
    plain_shader m_plain_shader;

	std::unordered_map<std::string, gl::Texture2dRef> m_textures;

	std::unordered_map<std::string, gl::BatchRef> m_batches;

	card_draft_intro m_intro;

    float eye_z = 15.0f;
};

void TestApp3::setup()
{
    setWindowSize({1024, 800});
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
}

void TestApp3::draw()
{
    gl::clear( ci::Color( 0.2f, 0.2f, 0.2f ) );

	m_cam.lookAt({0.0f, -1.0f, eye_z}, {0.0f, 0.0f, 0.0f});

	gl::setMatrices(m_cam);
	//gl::setMatrices(m_ortho);

	auto mouse_world = ci::gl::windowToWorldCoord(getMousePos() - getWindowPos());
	mouse_world.z = 1.25f;

    m_shader.set_light_position(mouse_world);

    m_intro.draw(*this);
}

CINDER_APP(TestApp3, ci::app::RendererGl)

