#type vertex
#version 460 core

const vec2 c_TexCoords[4] = vec2[4]( vec2(0, 0),
                                     vec2(1, 0),
                                     vec2(0, 1),
                                     vec2(1, 1) );

layout(std140, binding = 0) uniform Camera
{
  mat4 u_ViewProjection;
  vec3 u_CameraPosition;
};
layout(std140, binding = 1) uniform Block
{
  float u_BlockLength;
};
layout(std140, binding = 2) uniform Light
{
  float u_MaxSunlight;
  float u_SunIntensity;
};
layout(std430, binding = 0) buffer ChunkAnchors
{
  vec4 u_AnchorPosition[];
};

layout(location = 0) in uint a_VertexData;
layout(location = 1) in uint a_Lighting;

layout(location = 0) out flat uint v_TextureIndex;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 2) out vec4 v_BasicLight;

void main()
{
  // Relative position of block anchor
  vec3 relPos = u_BlockLength * vec3((a_VertexData >> 0 ) & 0x3F,
                                     (a_VertexData >> 6 ) & 0x3F,
                                     (a_VertexData >> 12) & 0x3F);

  uint quadIndex = (a_VertexData >> 18) & 0x3;
  v_TexCoord = c_TexCoords[quadIndex];
  v_TextureIndex = (a_VertexData >> 20) & 0x3FF;

  uint sunlightLevel =         (a_Lighting >> 16) & 0xF;
  uint ambientOcclusionLevel = (a_Lighting >> 20) & 0x3;

  float light = u_SunIntensity * float(sunlightLevel + 1) / (u_MaxSunlight + 1);
  light *= 1.0 - 0.2 * ambientOcclusionLevel;
  v_BasicLight = vec4(vec3(light), 1.0f);

  gl_Position = u_ViewProjection * vec4(u_AnchorPosition[gl_DrawID].xyz + relPos, 1.0f);
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