#type vertex
#version 450

layout(location = 0) in uint a_VertexData;

const uint s_TexSize = 128u;
const uint s_TexAtlasWidth = 1152u;
const uint s_TexAtlasHeight = 1280u;
const vec2 s_UVOffsets = vec2(float(s_TexSize) / s_TexAtlasWidth, float(s_TexSize) / s_TexAtlasHeight);

const vec2 s_TexOffsets[4] = vec2[4]( vec2(0.0f,		  0.0f         ),
									  vec2(s_UVOffsets.x, 0.0f         ),
									  vec2(s_UVOffsets.x, s_UVOffsets.y),
									  vec2(0.0f,		  s_UVOffsets.y) );

const float s_LightValues[6] = float[6]( 0.9f, 0.6f, 0.7f, 0.8f, 1.0f, 0.5f);

uniform float u_BlockLength;
uniform vec3 u_ChunkPosition;
uniform mat4 u_ViewProjection;

out vec2 v_TexCoord;
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
  const uint i = (a_VertexData & 0x7800000u) >> 23u;
  const uint j = (a_VertexData & 0xF8000000u) >> 27u;
  v_TexCoord.x = i * s_UVOffsets.x + s_TexOffsets[quadIndex].x;
  v_TexCoord.y = j * s_UVOffsets.y + s_TexOffsets[quadIndex].y;

  gl_Position = u_ViewProjection * vec4(relPos + u_ChunkPosition, 1.0f);
}



#type fragment
#version 450

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;
in vec4 v_BasicLight;

uniform sampler2D u_TextureAtlas;

void main()
{
  color = v_BasicLight * texture(u_TextureAtlas, v_TexCoord);
}