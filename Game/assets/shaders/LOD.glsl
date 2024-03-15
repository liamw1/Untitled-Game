#type vertex
#version 460 core

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
  vec3 u_CameraPosition;
};
layout(std430, binding = 0) buffer BlockTextureAverageColors
{
  vec4 u_AverageColor[][6];
};
layout(std430, binding = 2) buffer LODAnchors   // Maybe share this buffer with Chunk shader
{
  vec4 u_AnchorPosition[];
};

layout(binding = 0) uniform sampler2DArray u_TextureArray;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_IsoNormal;
layout(location = 2) in int  a_BlockIndex;

layout(location = 0) out vec3 v_LocalWorldPosition;
layout(location = 1) out vec4 v_Color;

vec3 vertexPosition = u_AnchorPosition[gl_DrawID].xyz + a_Position;
vec3 cameraToVertex = vertexPosition - u_CameraPosition;

vec4 calculateColorComponent(int coordID)
{
  // Block faces can only be seen if surface normal component is facing camera
  if (cameraToVertex[coordID] < 0 != a_IsoNormal[coordID] < 0)
  {
    int textureIndex = 2 * coordID + (cameraToVertex[coordID] < 0 ? 1 : 0);
    float colorStrength = abs(cameraToVertex[coordID] * a_IsoNormal[coordID]);
    return colorStrength * u_AverageColor[a_BlockIndex][textureIndex];
  }
  return vec4(0);
}

void main()
{
  vec4 xColor = calculateColorComponent(0);
  vec4 yColor = calculateColorComponent(1);
  vec4 zColor = calculateColorComponent(2);

  // Sum of color components needs to be normalized
  v_Color = (xColor + yColor + zColor) / (xColor.w + yColor.w + zColor.w);
  v_LocalWorldPosition = a_Position;
  gl_Position = u_ViewProjection * vec4(vertexPosition, 1.0f);
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