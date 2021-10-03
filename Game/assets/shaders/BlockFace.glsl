#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

const vec3 s_TexCoords[4] = { {0.0f, 0.0f},
                              {1.0f, 0.0f},
                              {1.0f, 1.0f},
                              {0.0f, 1.0f} };

uniform mat4 u_ViewProjection;

out vec2 v_TexCoord;

void main()
{
  v_TexCoord = a_TexCoord;
  gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}



#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

uniform sampler2D u_TextureAtlas;

void main()
{
  color = texture(u_TextureAtlas, v_TexCoord);
}