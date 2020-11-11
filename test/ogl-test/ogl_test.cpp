#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>

#include <filesystem>
#include <cassert>

#ifndef _USE_MATH_DEFINES
#   define USE_MATH_DEFINES
#endif

#include <cmath>

struct card_rect
{
    void setup(ci::app::AppBase* app)
    {
        ci::gl::Texture2d::Format format{};
        format.loadTopDown();

        auto img = ci::loadImage(app->loadAsset("card-hidden.png"));
        m_active_texture = ci::gl::Texture2d::create(img, format);

        m_hovered_overlay = ci::gl::Texture2d::create(ci::loadImage(app->loadAsset("card-highlight.png")), format);
        
        /*
        m_glsl = ci::gl::GlslProg::create(ci::gl::GlslProg::Format().vertex( CI_GLSL(330,
                uniform mat4 mvp;
                in vec4 pos;
                in vec2 in_tex_coord;
                out vec2 tex_coord;

                void main(void)
                {
                    gl_Position = mvp * pos;
                    tex_coord = in_tex_coord;
                }
                ))
            .fragment(CI_GLSL(330,
                out vec4 color;
                uniform sampler2D uTex0;

                in vec2 tex_coord;
                
                void main(void)
                {
                    color = texture(uTex0, tex_coord);
                }
            
            )));
            */

        auto shader = ci::gl::ShaderDef().texture().lambert();
        m_glsl = ci::gl::getStockShader(shader);

        //ci::gl::enableDepthWrite();
        //ci::gl::enableDepthRead();
        //ci::gl::enableAdditiveBlending();

        {
            ci::gl::ScopedTextureBind bind{m_active_texture, 0};
            m_batch = ci::gl::Batch::create(ci::geom::Rect{}, m_glsl);
        }
        {
            //ci::gl::ScopedTextureBind bind{m_active_texture, 0};
            ci::gl::ScopedTextureBind bind2{m_hovered_overlay, 0};
            m_hovered_batch = ci::gl::Batch::create(ci::geom::Rect{}, m_glsl);
        }
    }

    ci::gl::GlslProgRef m_glsl;

    template <typename T>
    void with_model(T const& fn)
    {
        auto w = 1.0f;
        auto h = w / m_active_texture->getAspectRatio();

        ci::gl::ScopedModelMatrix mat{};
        ci::gl::scale({w, h, 1.0f});
        //ci::gl::translate(getWindowCenter());
        ci::gl::translate(1.0f, 1.0f);

        fn();
    }

    void draw(ci::app::AppBase* app)
    {
        with_model([&]()
        {
            auto const coord = ci::gl::windowToObjectCoord(app->getMousePos() - app->getWindowPos());
            ci::Rectf r2{-0.5f, -0.5f, 0.5f, 0.5f};

            m_active_texture->bind();
            m_batch->draw();
            m_active_texture->unbind();

            if (!!m_hovered_overlay && r2.contains(coord))
            {
                m_hovered_overlay->bind();
                m_hovered_batch->draw();
                m_hovered_overlay->unbind();
            }
        });
    }

    float m_aspect_ratio = 90.0f / 60.0f;

    ci::gl::Texture2dRef m_active_texture;
    ci::gl::Texture2dRef m_hovered_overlay = nullptr;

    ci::gl::BatchRef m_batch;
    ci::gl::BatchRef m_hovered_batch;
};

class TestApp : public ci::app::App
{
public:
    void setup() override;

	//! Override to receive mouse-down events.
	void mouseDown(ci::app::MouseEvent event) override;

	//! Override to receive mouse-down events.
	void mouseUp(ci::app::MouseEvent event) override;

    void draw() override;
private:
    ci::gl::Texture2dRef m_tex;

    ci::gl::GlslProgRef m_glsl;

    ci::gl::BatchRef m_batch;

    ci::CameraPersp m_cam;

    card_rect m_card;
};

void TestApp::setup()
{
    m_cam.lookAt({0.0f, -2.0f, 10.0f}, {0.0f, 0.0f, 0.0f});
    m_cam.setWorldUp({0.0f, 0.0f, 1.0f});

#if 0
    //assert(std::filesystem::exists("card-hidden.png"));

    addAssetDirectory("../..");
    auto img = ci::loadImage(loadAsset("card-hidden.png"));
    ci::gl::Texture2d::Format form{};
    form.loadTopDown();
    m_tex = ci::gl::Texture2d::create(img, form);
    m_tex->bind();

    //auto shader = ci::gl::ShaderDef().texture().lambert();
    //m_glsl = ci::gl::getStockShader(shader);

    ci::gl::enableDepthWrite();
    ci::gl::enableDepthRead();
    ci::gl::enableAdditiveBlending();

    ci::geom::Rect r{};
    m_batch = ci::gl::Batch::create(r, m_glsl);
#endif


    m_card.setup(this);
}

//! Override to receive mouse-down events.
void TestApp::mouseDown(ci::app::MouseEvent event)
{

}

//! Override to receive mouse-down events.
void TestApp::mouseUp(ci::app::MouseEvent event)
{

}


void TestApp::draw()
{
    ci::gl::clear();
    ci::gl::setMatrices(m_cam);

    auto center = getWindowCenter();

    static float rad = 0.0f;

    m_card.draw(this);

    /*
    {
        ci::gl::ScopedModelMatrix mat{};
        ci::gl::scale({w, h, 1.0f});
        //ci::gl::translate(getWindowCenter());
        ci::gl::translate(1.0f, 1.0f);
        //ci::gl::rotate(rad*M_PI, {0.0f, 0.0f, 1.0f});
        rad += 0.002f;

        auto const coord = ci::gl::windowToObjectCoord(getMousePos() - getWindowPos());
        ci::Rectf r2{-0.5f, -0.5f, 0.5f, 0.5f};
        auto const contains = r2.contains(coord);
        if (contains)
        {
            //ci::gl::ScopedColor col{1.0f, 0.0f, 0.0f, 1.0f};
            //m_batch->draw();
        }
        else
        {
            m_batch->draw();
        }

    }
    */



}

//CINDER_APP(TestApp, ci::app::RendererGl)