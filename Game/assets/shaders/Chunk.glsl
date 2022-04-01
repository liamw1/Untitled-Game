#type vertex
#version 450 core

const vec2 s_TexCoords[4] = vec2[4]( vec2(0.0f,	0.0f),
									                   vec2(1.0f, 0.0f),
									                   vec2(1.0f, 1.0f),
									                   vec2(0.0f, 1.0f) );

const float s_LightValues[6] = float[6]( 0.9f, 0.6f, 0.7f, 0.8f, 1.0f, 0.5f);

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};
layout(std140, binding = 1) uniform Chunk
{
	vec3 u_AnchorPosition;
  float u_BlockLength;
};

layout(location = 0) in uint a_VertexData;

layout(location = 0) out flat uint v_TextureIndex;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 2) out vec4 v_BasicLight;

void main()
{
  // Relative position of block center
  vec3 relPos = vec3(u_BlockLength * float(a_VertexData & 0x3Fu),
						         u_BlockLength * float((a_VertexData & 0xFC0u) >> 6u),
						         u_BlockLength * float((a_VertexData & 0x3F000u) >> 12u));

  uint face = (a_VertexData & 0x1C0000u) >> 18u;
  v_BasicLight = vec4(vec3(s_LightValues[face]), 1.0f);

  uint quadIndex = (a_VertexData & 0x600000u) >> 21u;
  v_TextureIndex = (a_VertexData & 0xFF800000u) >> 23u;
  v_TexCoord = s_TexCoords[quadIndex];

  gl_Position = u_ViewProjection * vec4(u_AnchorPosition + relPos, 1.0f);
}



#type fragment
#version 450 core

layout(binding = 0) uniform sampler2DArray u_TextureArray;

layout(location = 0) in flat uint v_TextureIndex;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) in vec4 v_BasicLight;

layout(location = 0) out vec4 o_Color;

void main()
{
  o_Color = v_BasicLight * texture(u_TextureArray, vec3(v_TexCoord, v_TextureIndex));
  if (o_Color.a == 0)
	  discard;
}