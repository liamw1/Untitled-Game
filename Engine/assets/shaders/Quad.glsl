#type vertex
#version 450 core

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_TintColor;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in int a_TextureIndex;
layout(location = 4) in float a_TilingFactor;
layout(location = 5) in int a_EntityID;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out flat vec2 v_TexCoord;
layout(location = 2) out flat int v_TextureIndex;
layout(location = 3) out flat int v_EntityID;

void main()
{
  v_Color = a_TintColor;
  v_TexCoord = a_TexCoord * a_TilingFactor;
  v_TextureIndex = a_TextureIndex;
  v_EntityID = a_EntityID;

  gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}



#type fragment
#version 450 core

layout(binding = 0) uniform sampler2D u_Textures[32];

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) in flat int v_TextureIndex;
layout(location = 3) in flat int v_EntityID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

void main()
{
  o_Color = texture(u_Textures[v_TextureIndex], v_TexCoord) * v_Color;
  o_EntityID = v_EntityID;
}