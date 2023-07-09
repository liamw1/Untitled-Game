#type vertex
#version 460 core

layout(location = 0) in uint a_VoxelData;
layout(location = 1) in uint a_QuadData1;
layout(location = 2) in uint a_QuadData2;

layout(location = 0) out uint v_VoxelData;
layout(location = 1) out uint v_QuadData1;
layout(location = 2) out uint v_QuadData2;
layout(location = 3) out uint v_DrawID;

void main()
{
  v_VoxelData = a_VoxelData;
  v_QuadData1 = a_QuadData1;
  v_QuadData2 = a_QuadData2;
  v_DrawID = gl_DrawID;
}



#type geometry
#version 460 core

const vec2 c_TexCoords[4] = vec2[4]( vec2(0, 0),
                                     vec2(1, 0),
                                     vec2(0, 1),
                                     vec2(1, 1) );

const ivec3 c_Directions[6] = ivec3[6]( ivec3( -1,  0,  0 ),
                                        ivec3(  1,  0,  0 ),
                                        ivec3(  0, -1,  0 ),
                                        ivec3(  0,  1,  0 ),
                                        ivec3(  0,  0, -1 ),
                                        ivec3(  0,  0,  1 ) );

const uvec3 c_Offsets[6][4] = uvec3[6][4]( uvec3[4]( uvec3(0, 1, 0), uvec3(0, 0, 0), uvec3(0, 1, 1), uvec3(0, 0, 1) ),    /*  West Face   */
                                           uvec3[4]( uvec3(1, 0, 0), uvec3(1, 1, 0), uvec3(1, 0, 1), uvec3(1, 1, 1) ),    /*  East Face   */
                                           uvec3[4]( uvec3(0, 0, 0), uvec3(1, 0, 0), uvec3(0, 0, 1), uvec3(1, 0, 1) ),    /*  South Face  */
                                           uvec3[4]( uvec3(1, 1, 0), uvec3(0, 1, 0), uvec3(1, 1, 1), uvec3(0, 1, 1) ),    /*  North Face  */
                                           uvec3[4]( uvec3(0, 1, 0), uvec3(1, 1, 0), uvec3(0, 0, 0), uvec3(1, 0, 0) ),    /*  Bottom Face */
                                           uvec3[4]( uvec3(0, 0, 1), uvec3(1, 0, 1), uvec3(0, 1, 1), uvec3(1, 1, 1) ) );  /*  Top Face    */

layout(std140, binding = 0) uniform Camera
{
  mat4 u_ViewProjection;
  vec3 u_CameraPosition;
};
layout(std140, binding = 1) uniform Block
{
  float u_BlockLength;
};
layout(std140, binding = 2) uniform RenderPass
{
  bool u_TransparencyPass;
};
layout(std430, binding = 0) buffer ChunkAnchors
{
  vec4 u_AnchorPosition[];
};
layout(std430, binding = 1) buffer TextureIDs
{
  uint u_TextureIDs[][6];
};

layout (points) in;
layout (triangle_strip, max_vertices = 24) out;   // max_vertices could be reduced to 12 for opaque voxels

layout(location = 0) in uint v_VoxelData[];
layout(location = 1) in uint v_QuadData1[];
layout(location = 2) in uint v_QuadData2[];
layout(location = 3) in uint v_DrawID[];

layout(location = 0) out float g_BasicLight;
layout(location = 1) out flat uint g_TextureIndex;
layout(location = 2) out vec2 g_TexCoord;

bool isUpstream(uint faceID)
{
  return faceID % 2 > 0;
}

bool isCulled(uint faceID, vec3 toBlock)
{
  return dot(toBlock, c_Directions[faceID]) > 0;
}

bool faceEnabled(uint faceID)
{
  uint bit = 1 << (16 + faceID);
  return (v_QuadData2[0] & bit) > 0;
}

uint vertexAmbientOcclusionLevel(uint faceID, uint quadIndex)
{
  uint shift = 2 * (4 * faceID + quadIndex);
  if (shift < 32)
  {
    uint bits = 0x00000003u << shift;
    return (v_QuadData1[0] & bits) >> shift;
  }
  else
  {
    shift -= 32;
    uint bits = 0x00000003u << shift;
    return (v_QuadData2[0] & bits) >> shift;
  }
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

void addVertex(uvec3 blockIndex, uint faceID, uint quadIndex, uint ambientOcclusionLevel, uint textureID)
{
  uvec3 vertexOffset = c_Offsets[faceID][quadIndex];
  vec3 relativeVertexPosition = u_BlockLength * vec3(blockIndex + vertexOffset);
  
  g_BasicLight = 1.0 - 0.2 * ambientOcclusionLevel;
  g_TextureIndex = textureID;
  g_TexCoord = c_TexCoords[quadIndex];
  
  gl_Position = u_ViewProjection * vec4(u_AnchorPosition[v_DrawID[0]].xyz + relativeVertexPosition, 1.0f);
  EmitVertex();
}

void addQuad(uvec3 blockIndex, uint faceID, uint textureID)
{
  uvec4 AO = ambientOcclusionLevels(faceID);

  uint lightDifferenceAlongStandardSeam = min(AO[0] - AO[3], AO[3] - AO[0]);
  uint lightDifferenceAlongReversedSeam = min(AO[1] - AO[2], AO[2] - AO[1]);

  // Choose vertex ordering that minimizes difference in light across seam
  uvec4 quadOrder = lightDifferenceAlongStandardSeam >= lightDifferenceAlongReversedSeam ? uvec4(0, 1, 2, 3) : uvec4(1, 3, 0, 2);
  for (int i = 0; i < 4; ++i)
  {
    uint quadIndex = quadOrder[i];
    addVertex(blockIndex, faceID, quadIndex, AO[quadIndex], textureID);
  }
  EndPrimitive();
}

void main()
{
  uint blockID = v_VoxelData[0] & 0x0000FFFFu;
  uvec3 blockIndex = uvec3((v_VoxelData[0] & 0x001F0000u) >> 16u,
                           (v_VoxelData[0] & 0x03E00000u) >> 21u,
                           (v_VoxelData[0] & 0x7C000000u) >> 26u);

  vec3 blockCenter = u_AnchorPosition[v_DrawID[0]].xyz + u_BlockLength * vec3(blockIndex) + vec3(u_BlockLength) / 2;
  vec3 toBlock = blockCenter - u_CameraPosition.xyz;

  for (uint coordID = 0; coordID < 3; ++coordID)
  {
    uint faceID = 2 * coordID + (toBlock[coordID] < 0 ? 1 : 0);
    if (faceEnabled(faceID))
      addQuad(blockIndex, faceID, u_TextureIDs[blockID][faceID]);
  }

  if (u_TransparencyPass)
    for (uint coordID = 0; coordID < 3; ++coordID)
    {
      uint faceID = 2 * coordID + (toBlock[coordID] < 0 ? 0 : 1);
      if (faceEnabled(faceID))
        addQuad(blockIndex, faceID, u_TextureIDs[blockID][faceID]);
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