#include "shader.h"
#include <cassert>

namespace aura
{

void phong_shader::setup() noexcept
{
	using namespace ci;

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
		uniform vec3        ambient_light;

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

			vec3 ambient = ambient_light;//vec3(0.3, 0.3, 0.3);

			float specular_strength = 0.8;
			vec3 viewDir = normalize(vec3(0.0, -1.0, 10) - worldCoord);
			vec3 reflectDir = reflect(-lightDir, norm);
			float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
			vec3 specular = specular_strength * spec * vec3(1.0, 1.0, 1.0);

			color = vec4(ambient + diffuse + specular, 1.0) * texture(uTex0, TexCoord0);
		}
	) ) );

	m_glsl->bind();
	m_glsl->uniform("uTex0", 0);
	m_glsl->uniform("uNormalMap", 1);
	set_ambient_light({0.5f, 0.5f, 0.5f});
}

void phong_shader::set_light_position(ci::vec3 const& pos) noexcept
{
	m_glsl->uniform("light0", pos);
}

void phong_shader::set_ambient_light(ci::ColorAf col) noexcept
{
	m_glsl->uniform("ambient_light", ci::vec3{col.r, col.g, col.b});
}

//! normal_map may be null
void phong_shader::draw(ci::gl::Texture2dRef texture,
                        ci::gl::Texture2dRef normal_map,
                        std::vector<ci::gl::BatchRef> targets) noexcept
{
	using namespace ci;

	assert(texture);

	m_glsl->bind();

	if (normal_map)
		m_glsl->uniform("use_normal_map", true);

	gl::ScopedTextureBind bind{texture};
	//gl::ScopedTextureBind b2{normal_map, 1};
	if (normal_map)
		normal_map->bind(1);

	for (auto const& t : targets)
		t->draw();

	if (normal_map)
		normal_map->unbind(1);
}

void plain_shader::setup() noexcept
{
	using namespace ci;

    m_glsl = gl::GlslProg::create( gl::GlslProg::Format()
	.vertex(	CI_GLSL( 330,
		uniform mat4    ciModelMatrix;
		uniform mat4	ciViewProjection;

		in vec4			ciPosition;
		
		void main( void ) {
			gl_Position	= ciViewProjection * ciModelMatrix * ciPosition;
		}
	 ) )
	.fragment(	CI_GLSL( 330,
		uniform vec4		uColor;

		out vec4			color;

		void main( void ) {
			color = uColor;
		}
	) ) );

	m_glsl->bind();
}

//! normal_map may be null
void plain_shader::draw(ci::ColorAf const& color,
                      std::vector<ci::gl::BatchRef> targets) noexcept

{
	using namespace ci;

	m_glsl->bind();

	//gl::ScopedColor col{color};
	m_glsl->uniform("uColor", color);

	for (auto const& t : targets)
		t->draw();
}

} // namespace aura
