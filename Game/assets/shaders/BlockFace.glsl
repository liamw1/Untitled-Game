#type vertex
#version 450 core

layout(location = 0) in uint a_VertexData;

const vec2 s_TexCoords[4] = vec2[4]( vec2(0.0f,	0.0f),
									 vec2(1.0f, 0.0f),
									 vec2(1.0f, 1.0f),
									 vec2(0.0f, 1.0f) );

const float s_LightValues[6] = float[6]( 0.9f, 0.6f, 0.7f, 0.8f, 1.0f, 0.5f);

uniform float u_BlockLength;
uniform vec3 u_ChunkPosition;
uniform mat4 u_ViewProjection;

out vec3 v_TexCoord;
out vec4 v_BasicLight;

void main()
{
  // Relative position of block center
  const vec3 relPos = vec3(u_BlockLength * float(a_VertexData & 0x3Fu),
						   u_BlockLength * float((a_VertexData & 0xFC0u) >> 6u),
						   u_BlockLength * float((a_VertexData & 0x3F000u) >> 12u));

  const uint face = (a_VertexData & 0x1C0000u) >> 18u;
  v_BasicLight = vec4(vec3(s_LightValues[face]), 1.0f);

  const uint quadIndex = (a_VertexData & 0x600000u) >> 21u;
  const uint layer = (a_VertexData & 0xFF800000u) >> 23u;
  v_TexCoord = vec3(s_TexCoords[quadIndex], float(layer));

  gl_Position = u_ViewProjection * vec4(u_ChunkPosition + relPos, 1.0f);
}



#type fragment
#version 450 core

layout(location = 0) out vec4 color;

in vec3 v_TexCoord;
in vec4 v_BasicLight;

uniform sampler2DArray u_TextureArray;

void main()
{
  color = v_BasicLight * texture(u_TextureArray, v_TexCoord);
  if (color.a == 0)
	discard;
}