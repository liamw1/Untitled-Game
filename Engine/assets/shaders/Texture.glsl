#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_TintColor;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TextureIndex;
layout(location = 4) in float a_ScalingFactor;

uniform mat4 u_ViewProjection;

out vec4 v_TintColor;
out vec2 v_TexCoord;
out float v_TextureIndex;

void main()
{
  v_TintColor = a_TintColor;
  v_TexCoord = a_TexCoord * a_ScalingFactor;
  v_TextureIndex = a_TextureIndex;
  gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}



#type fragment
#version 330 core

layout(location = 0) out vec4 color;
layout(location = 1) out int entityID;

in vec4 v_TintColor;
in vec2 v_TexCoord;
in float v_TextureIndex;

uniform sampler2D u_Textures[32];

void main()
{
  color = texture(u_Textures[int(v_TextureIndex)], v_TexCoord) * v_TintColor;
  entityID = 50;
}