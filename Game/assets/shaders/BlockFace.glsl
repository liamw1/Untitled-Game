#type vertex
#version 450

layout(location = 0) in uint a_VertexData;

const float s_BlockSize = 0.2f;

const vec2 s_TexCoords[4] = vec2[4]( vec2(0.0f, 0.0f),
									 vec2(1.0f, 0.0f),
									 vec2(1.0f, 1.0f),
									 vec2(0.0f, 1.0f) );

const float s_LightValues[6] = float[6]( 1.0f, 0.5f, 0.7f, 0.8f, 0.9f, 0.6f);

uniform vec3 u_ChunkPosition;
uniform mat4 u_ViewProjection;

out vec2 v_TexCoord;
out vec4 v_BasicLight;

void main()
{
  // Relative position of block center
  vec3 relPos = vec3(s_BlockSize * float(a_VertexData & 0x3Fu),
                     s_BlockSize * float((a_VertexData & 0xFC0u) >> 6u),
                     s_BlockSize * float((a_VertexData & 0x3F000u) >> 12u));

  uint face = (a_VertexData & 0x1C0000u) >> 18u;
  v_BasicLight = vec4(vec3(s_LightValues[face]), 1.0f);

  uint quadIndex = (a_VertexData & 0x600000u) >> 21u;
  uint texID = (a_VertexData & 0xFF800000u) >> 23u;
  v_TexCoord = s_TexCoords[quadIndex];

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