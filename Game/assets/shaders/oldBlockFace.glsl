#type vertex
#version 450

layout(location = 0) in uint a_VertexData;

const float s_BlockSize = 0.2f;

const vec3 s_BlockFacePositions[24] 
= vec3[24]( // Top Face
            vec3(-s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2),
            vec3( s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2),
            vec3( s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2),
            vec3(-s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2),
            
            // Bottom Face
            vec3(-s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2),
            vec3( s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2),
            vec3( s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2),
            vec3(-s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2),
            
            // North Face
            vec3(-s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2),
            vec3( s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2),
            vec3( s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2),
            vec3(-s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2),
            
            // South Face
            vec3( s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2),
            vec3(-s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2),
            vec3(-s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2),
            vec3( s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2),
            
            // East Face
            vec3(-s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2),
            vec3(-s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2),
            vec3(-s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2),
            vec3(-s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2),
            
            // West Face
            vec3( s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2),
            vec3( s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2),
            vec3( s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2),
            vec3( s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2) );
               
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
  // Relative position of block
  float x = s_BlockSize * float(a_VertexData & 0x1Fu);
  float y = s_BlockSize * float((a_VertexData & 0x3E0u) >> 5u);
  float z = s_BlockSize * float((a_VertexData & 0x7C00u) >> 10u);

  uint face = (a_VertexData & 0x38000u) >> 15u;
  v_BasicLight = vec4(vec3(s_LightValues[face]), 1.0f);

  uint quadIndex = (a_VertexData & 0xC0000u) >> 18u;
  uint texID = (a_VertexData & 0xFFF00000u) >> 20u;
  v_TexCoord = s_TexCoords[quadIndex];

  x += u_ChunkPosition.x + s_BlockFacePositions[4u * face + quadIndex].x;
  y += u_ChunkPosition.y + s_BlockFacePositions[4u * face + quadIndex].y;
  z += u_ChunkPosition.z + s_BlockFacePositions[4u * face + quadIndex].z;
  gl_Position = u_ViewProjection * vec4(x, y, z, 1.0f);
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