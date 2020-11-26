#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>

#include <filesystem>
#include <cassert>
#include <cstdlib>

using namespace ci;

class TestApp2 : public ci::app::App
{
public:
    void setup() override;

    void draw() override;
private:
    ci::CameraPersp m_cam;

    gl::GlslProgRef m_glsl;

	gl::BatchRef m_batch;

	gl::BatchRef m_test_sphere;

	gl::Texture2dRef m_texture;

	gl::Texture2dRef m_hovered_texture;

	gl::Texture2dRef m_hidden_texture;

	gl::Texture2dRef m_hidden_normal_map;

	gl::Texture2dRef m_card_normal_map;
};

void TestApp2::setup()
{
	m_cam.lookAt({0.0f, -1.0f, 10.0f}, {0.0f, 0.0f, 0.0f});

    m_glsl = gl::GlslProg::create( gl::GlslProg::Format()
	.vertex(	CI_GLSL( 330,
		uniform mat4    ciModelMatrix;
		uniform mat4	ciViewProjection;
		uniform mat3    ciNormalMatrix;
		in vec4			ciPosition;
		in vec2			ciTexCoord0;
		in vec3         ciNormal;
		in vec3		    ciTangent;
		in vec3         ciBitangent;
		out vec2	    TexCoord0;
		out vec3        normal;
		out vec3        worldCoord;
		out mat3        TBN;
		
		void main( void ) {
			gl_Position	= ciViewProjection * ciModelMatrix * ciPosition;
			TexCoord0 = ciTexCoord0;
			normal = ciNormalMatrix * ciNormal;
			worldCoord = vec3(ciModelMatrix * ciPosition);

			vec3 T = normalize(vec3(ciModelMatrix * vec4(ciTangent,   0.0)));
            vec3 B = normalize(vec3(ciModelMatrix * vec4(ciBitangent, 0.0)));
            vec3 N = normalize(vec3(ciModelMatrix * vec4(ciNormal,    0.0)));
			TBN = mat3(T, B, N);
		}
	 ) )
	.fragment(	CI_GLSL( 330,
		uniform vec4		uColor;
		uniform sampler2D   uTex0;
		uniform sampler2D   uNormalMap;
		uniform vec3        light0;
		uniform bool		use_normal_map;

		in vec2				TexCoord0;
		in vec3             normal;
		in vec3             worldCoord;
		in mat3             TBN;
		out vec4			color;
		
		vec3 calculate_norm()
		{
			if (use_normal_map)
			{
				float normal_strength = 1.0;

                vec3 norm = texture(uNormalMap, TexCoord0).rgb;
                norm = ((norm * 2.0) - 1.0)*normal_strength;
                //norm = normalize(TBN * norm);
				return norm;
			}
			return normalize(normal);
		}

		void main( void ) {
			vec3 norm = calculate_norm();

			vec3 lightDir = normalize(light0 - worldCoord);
			float diff = max(dot(norm, lightDir), 0.0);
			vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

			vec3 ambient = vec3(0.3, 0.3, 0.3);

			float specular_strength = 0.5;
			vec3 viewDir = normalize(vec3(0.0, -1.0, 10) - worldCoord);
			vec3 reflectDir = reflect(-lightDir, norm);
			float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
			vec3 specular = specular_strength * spec * vec3(1.0, 1.0, 1.0);

			color = vec4(ambient + diffuse + specular, 1.0) * texture(uTex0, TexCoord0);
		}
	) ) );

    ci::gl::Texture2d::Format format{};
    format.loadTopDown();

    auto img = ci::loadImage(loadAsset("card-Potion of Speed.png"));
    m_texture = ci::gl::Texture2d::create(img, format);

	m_hovered_texture = ci::gl::Texture2d::create(ci::loadImage(loadAsset("card-highlight.png")), format);
	m_hidden_texture = ci::gl::Texture2d::create(ci::loadImage(loadAsset("card-hidden.png")), format);
	m_hidden_normal_map = ci::gl::Texture2d::create(ci::loadImage(loadAsset("normal-hidden.png")), format);
	m_card_normal_map = ci::gl::Texture2d::create(ci::loadImage(loadAsset("normalmap-card.png")), format);

	m_batch = gl::Batch::create(ci::geom::Rect(), m_glsl);
	m_test_sphere = gl::Batch::create(ci::geom::Sphere(), m_glsl);

	auto const& a = m_glsl->getActiveAttributes();
	for (auto const& b : a)
	{
		b.getName();
	}

	gl::enableDepthWrite();
	gl::enableDepthRead();
	gl::enableAlphaBlending();
	srand(time(nullptr));
}

void TestApp2::draw()
{
    gl::clear( ci::Color( 0.2f, 0.2f, 0.2f ) );
	gl::setMatrices(m_cam);

	//static vec2 light_pos(getWindowCenter());
	//
	//light_pos.x += 0.5*((rand() % 3) - 1); // 0, 1, 2 -> -1 0 1
	//light_pos.y += 0.5*((rand() % 3) - 1); // 0, 1, 2 -> -1 0 1

	auto mouse_world = ci::gl::windowToWorldCoord(getMousePos() - getWindowPos());
	mouse_world.z = 1.25f;

	m_glsl->uniform("uColor", ColorAf{1.0f, 1.0f, 1.0f, 1.0f});
	m_glsl->uniform("light0", mouse_world);
	//m_glsl->uniform("light0", ci::gl::windowToWorldCoord(light_pos));

    m_glsl->bind();
    m_glsl->uniform("uTex0", 0);
    m_glsl->uniform("uNormalMap", 1);
    m_glsl->uniform("use_normal_map", true);

	static float rad = 0.00f;
	rad += 0.01f;

	{
        gl::ScopedTextureBind bind{m_texture};
		gl::ScopedTextureBind b2{m_card_normal_map, 1};

        auto w = 4*(0.7f);
        auto h = 4.0f;

        ci::gl::ScopedModelMatrix mat{};
        ci::gl::scale({w, h, 1.0f});
		ci::gl::rotate(rad, {0.0f, 1.0f, 0.0f});

        m_batch->draw();
	}

	{
        gl::ScopedTextureBind bind{m_hidden_texture};
        gl::ScopedTextureBind b2{m_hidden_normal_map, 1};
		//m_hidden_normal_map->bind(1);

        auto w = 4*(0.7f);
        auto h = 4.0f;

        ci::gl::ScopedModelMatrix mat{};
        ci::gl::scale({w, h, 1.0f});
		ci::gl::rotate(rad + M_PI, {0.0f, 1.0f, 0.0f});
		ci::gl::translate(0.0f, 0.0f, 0.01f);

        m_batch->draw();

		m_glsl->uniform("use_normal_map", false);
	}

	{
        gl::ScopedTextureBind bind{m_texture};
        ci::gl::ScopedModelMatrix mat{};
        //ci::gl::scale({w, h, 1.0f});
        ci::gl::translate(2.0f, 2.0f, 0.1f);
		m_test_sphere->draw();
	}
}

//CINDER_APP(TestApp2, ci::app::RendererGl)