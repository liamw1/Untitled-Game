#type vertex
#version 460 core

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};
layout(std430, binding = 0) buffer BlockTextureAverageColors
{
  vec4 u_AverageColor[];
};
layout(std430, binding = 2) buffer LODAnchors   // Maybe share this buffer with Chunk shader
{
  vec4 u_AnchorPosition[];
};

layout(binding = 0) uniform sampler2DArray u_TextureArray;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_IsoNormal;
layout(location = 2) in int  a_TextureIndex;

layout(location = 0) out vec3 v_LocalWorldPosition;
layout(location = 1) out vec4 v_Color;

void main()
{
  v_LocalWorldPosition = a_Position;
  gl_Position = u_ViewProjection * vec4(u_AnchorPosition[gl_DrawID].xyz + a_Position, 1.0f);
  v_Color = u_AverageColor[a_TextureIndex];
}



#type fragment
#version 460 core

layout(location = 0) in vec3 v_LocalWorldPosition;
layout(location = 1) in vec4 v_Color;

layout(location = 0) out vec4 o_Color;

void main()
{
  vec3 normal = -normalize(cross(dFdy(v_LocalWorldPosition), dFdx(v_LocalWorldPosition)));
  float lightValue = (1.0 + normal.z) / 2;

  o_Color = vec4(vec3(lightValue), 1.0f) * v_Color;
}