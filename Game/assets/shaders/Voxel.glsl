#type vertex
#version 460 core

const vec2 s_TexCoords[4] = vec2[4]( vec2(0.0f, 0.0f),
                                     vec2(1.0f, 0.0f),
                                     vec2(1.0f, 1.0f),
                                     vec2(0.0f, 1.0f) );

const float s_AO[4] = float[4](0.4f, 0.6f, 0.8f, 1.0f);

layout(std140, binding = 0) uniform Camera
{
  mat4 u_ViewProjection;
};
layout(std140, binding = 1) uniform Block
{
  float u_BlockLength;
};
layout(std430, binding = 0) buffer ChunkAnchors
{
  vec4 u_AnchorPosition[];
};

layout(location = 0) in uint a_VertexData;

layout(location = 0) out flat uint v_TextureIndex;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 2) out vec4 v_BasicLight;

void main()
{
  // Relative position of block anchor
  vec3 relPos = u_BlockLength * vec3((a_VertexData & 0x0000003Fu) >> 0u,
                                     (a_VertexData & 0x00000FC0u) >> 6u,
                                     (a_VertexData & 0x0003F000u) >> 12u);

  uint quadIndex = (a_VertexData & 0x000C0000u) >> 18u;
  uint AOIndex   = (a_VertexData & 0x00300000u) >> 20u;
  v_TextureIndex = (a_VertexData & 0xFFC00000u) >> 22u;

  v_TexCoord = s_TexCoords[quadIndex];
  v_BasicLight = vec4(vec3(s_AO[AOIndex]), 1.0f);

  gl_Position = u_ViewProjection * vec4(u_AnchorPosition[gl_DrawID].xyz + relPos, 1.0f);
}



#type geometry
#version 460 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout(location = 0) in flat uint v_TextureIndex[];
layout(location = 1) in vec2 v_TexCoord[];
layout(location = 2) in vec4 v_BasicLight[];

layout(location = 0) out flat uint g_TextureIndex;
layout(location = 1) out vec2 g_TexCoord;
layout(location = 2) out vec4 g_BasicLight;

void addVertex(int vertexIndex)
{
  g_TextureIndex = v_TextureIndex[vertexIndex];
  g_TexCoord = v_TexCoord[vertexIndex];
  g_BasicLight = v_BasicLight[vertexIndex];
  
  gl_Position = gl_in[vertexIndex].gl_Position; 
  EmitVertex();
}

void main() {
    for (int v = 0; v < 3; ++v)
      addVertex(v);
    EndPrimitive();
}  



#type fragment
#version 460 core

layout(binding = 0) uniform sampler2DArray u_TextureArray;

layout(location = 0) in flat uint v_TextureIndex;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) in vec4 v_BasicLight;

layout(location = 0) out vec4 o_Color;

void main()
{
  o_Color = v_BasicLight * texture(u_TextureArray, vec3(v_TexCoord, v_TextureIndex));
}