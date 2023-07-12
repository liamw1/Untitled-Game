#type vertex
#version 460 core

layout(std430, binding = 0) buffer ChunkAnchors
{
  vec4 u_AnchorPosition[];
};
layout(std430, binding = 1) buffer TextureIDs
{
  uint u_TextureIDs[][6];
};

layout(location = 0) in uint a_VoxelData;
layout(location = 1) in uint a_QuadData1;
layout(location = 2) in uint a_QuadData2;
layout(location = 3) in uint a_Sunlight;

layout(location = 0) out uint v_VoxelData;
layout(location = 1) out uint v_QuadData1;
layout(location = 2) out uint v_QuadData2;
layout(location = 3) out uint v_Sunlight;
layout(location = 4) out vec3 v_ChunkAnchor;
layout(location = 5) out uint[6] v_TextureIDs;

void main()
{
  uint blockID = a_VoxelData & 0x0000FFFFu;

  v_VoxelData = a_VoxelData;
  v_QuadData1 = a_QuadData1;
  v_QuadData2 = a_QuadData2;
  v_Sunlight = a_Sunlight;
  v_ChunkAnchor = u_AnchorPosition[gl_DrawID].xyz;
  v_TextureIDs = u_TextureIDs[blockID];
}



#type geometry
#version 460 core

#define TRANSPARENT

#if TRANSPARENT
  #define MAX_VERTICES 24
#else
  #define MAX_VERTICES 12
#endif

const vec2 c_TexCoords[4] = vec2[4]( vec2(0, 0),
                                     vec2(1, 0),
                                     vec2(0, 1),
                                     vec2(1, 1) );

const uint c_VertexIndices[6][4] = uint[6][4]( uint[4](2, 0, 3, 1),     /*  West Face   */
                                               uint[4](4, 6, 5, 7),     /*  East Face   */
                                               uint[4](0, 4, 1, 5),     /*  South Face  */
                                               uint[4](6, 2, 7, 3),     /*  North Face  */
                                               uint[4](2, 6, 0, 4),     /*  Bottom Face */
                                               uint[4](1, 5, 3, 7) );   /*  Top Face    */

layout(std140, binding = 0) uniform Camera
{
  mat4 u_ViewProjection;
  vec3 u_CameraPosition;
};
layout(std140, binding = 1) uniform Block
{
  float u_BlockLength;
};

layout (points) in;
layout (triangle_strip, max_vertices = MAX_VERTICES) out;

layout(location = 0) in uint v_VoxelData[];
layout(location = 1) in uint v_QuadData1[];
layout(location = 2) in uint v_QuadData2[];
layout(location = 3) in uint v_Sunlight[];
layout(location = 4) in vec3 v_ChunkAnchor[];
layout(location = 5) in uint[6] v_TetureIDs[];

layout(location = 0) out float g_BasicLight;
layout(location = 1) out flat uint g_TextureIndex;
layout(location = 2) out vec2 g_TexCoord;

bool faceEnabled(uint faceID)
{
  uint bit = 1 << (16 + faceID);
  return (v_QuadData2[0] & bit) > 0;
}

uvec4 ambientOcclusionLevels(uint faceID)
{
  uint shift = 8 * faceID;

  uint quadAO;
  if (shift < 32)
  {
    uint bits = 0x000000FFu << shift;
    quadAO = (v_QuadData1[0] & bits) >> shift;
  }
  else
  {
    shift -= 32;
    uint bits = 0x000000FFu << shift;
    quadAO = (v_QuadData2[0] & bits) >> shift;
  }

  return uvec4((quadAO & 0x00000003) >> 0,
               (quadAO & 0x0000000C) >> 2,
               (quadAO & 0x00000030) >> 4,
               (quadAO & 0x000000C0) >> 6);
}

uvec3 computeVertexOffset(uint vertexIndex)
{
  return uvec3((vertexIndex & 0x00000004u) >> 2,
               (vertexIndex & 0x00000002u) >> 1,
               (vertexIndex & 0x00000001u) >> 0);
}

float computeLight(uint vertexIndex, uint ambientOcclusionLevel)
{
  uint shift = 4 * vertexIndex;
  uint bits = 0x0000000Fu << shift;
  uint sunlightIntensity = (v_Sunlight[0] & bits) >> shift;

  float lightVal = float(sunlightIntensity + 1) / 16.0;
  return lightVal * (1.0 - 0.2 * ambientOcclusionLevel);
}

void addVertex(uvec3 blockIndex, uint faceID, uint quadIndex, uint ambientOcclusionLevel, uint textureID)
{
  uint vertexIndex = c_VertexIndices[faceID][quadIndex];
  vec3 relativeVertexPosition = u_BlockLength * vec3(blockIndex + computeVertexOffset(vertexIndex));
  
  g_BasicLight = computeLight(vertexIndex, ambientOcclusionLevel);
  g_TextureIndex = textureID;
  g_TexCoord = c_TexCoords[quadIndex];
  
  gl_Position = u_ViewProjection * vec4(v_ChunkAnchor[0] + relativeVertexPosition, 1.0f);
  EmitVertex();
}

void addQuad(uvec3 blockIndex, uint faceID, uint textureID)
{
  uvec4 AO = ambientOcclusionLevels(faceID);

  uint lightDifferenceAlongStandardSeam = min(AO[0] - AO[3], AO[3] - AO[0]);
  uint lightDifferenceAlongReversedSeam = min(AO[1] - AO[2], AO[2] - AO[1]);

  // Choose vertex ordering that minimizes difference in light across seam
  uvec4 quadOrder = uvec4(0, 1, 2, 3);
  if (lightDifferenceAlongStandardSeam < lightDifferenceAlongReversedSeam)
  {
    quadOrder = uvec4(1, 3, 0, 2);
    AO = uvec4(AO[1], AO[3], AO[0], AO[2]);
  }

  for (int i = 0; i < 4; ++i)
    addVertex(blockIndex, faceID, quadOrder[i], AO[i], textureID);

  EndPrimitive();
}

void main()
{
  uvec3 blockIndex = uvec3((v_VoxelData[0] & 0x001F0000u) >> 16u,
                           (v_VoxelData[0] & 0x03E00000u) >> 21u,
                           (v_VoxelData[0] & 0x7C000000u) >> 26u);

  vec3 blockCenter = v_ChunkAnchor[0] + u_BlockLength * vec3(blockIndex) + vec3(u_BlockLength) / 2;
  vec3 toBlock = blockCenter - u_CameraPosition.xyz;

#if TRANSPARENT
  for (uint coordID = 0; coordID < 3; ++coordID)
  {
    uint faceID = 2 * coordID + (toBlock[coordID] < 0 ? 0 : 1);
    if (faceEnabled(faceID))
      addQuad(blockIndex, faceID, v_TetureIDs[0][faceID]);
  }
#endif

  for (uint coordID = 0; coordID < 3; ++coordID)
  {
    uint faceID = 2 * coordID + (toBlock[coordID] < 0 ? 1 : 0);
    if (faceEnabled(faceID))
      addQuad(blockIndex, faceID, v_TetureIDs[0][faceID]);
  }
}



#type fragment
#version 460 core

layout(binding = 0) uniform sampler2DArray u_TextureArray;

layout(location = 0) in float g_BasicLight;
layout(location = 1) in flat uint g_TextureIndex;
layout(location = 2) in vec2 g_TexCoord;

layout(location = 0) out vec4 o_Color;

void main()
{
  vec4 light = vec4(vec3(g_BasicLight), 1.0);
  o_Color = light * texture(u_TextureArray, vec3(g_TexCoord, g_TextureIndex));
}