#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in int a_QuadIndex;
layout(location = 2) in float a_LightValue;

const vec2 s_TexCoords[4] = vec2[4]( vec2(0.0f,	0.0f),
									 vec2(1.0f, 0.0f),
									 vec2(1.0f, 1.0f),
									 vec2(0.0f, 1.0f) );

uniform float u_TextureScaling;
uniform vec3 u_LODPosition;
uniform mat4 u_ViewProjection;

out float v_LightValue;
out vec3 v_TexCoord;

void main()
{
  v_LightValue = a_LightValue;
  v_TexCoord = vec3(u_TextureScaling * s_TexCoords[a_QuadIndex], float(1));
  gl_Position = u_ViewProjection * vec4(u_LODPosition + a_Position, 1.0f);
}



#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in float v_LightValue;
in vec3 v_TexCoord;

uniform sampler2DArray u_TextureArray;

void main()
{
  color = vec4(vec3(v_LightValue), 1.0f) * texture(u_TextureArray, v_TexCoord);
  if (color.a == 0)
	discard;
}