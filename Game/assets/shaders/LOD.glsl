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

layout(location = 0) out vec3 v_LocalPosition;
layout(location = 1) out vec3 v_VertexToCamera;
layout(location = 2) out mat4 v_ColorComponents;

void main()
{
  vec3 vertexPosition = u_AnchorPosition[gl_DrawID].xyz + a_Position;

  v_LocalPosition = a_Position;
  v_VertexToCamera = u_CameraPosition - vertexPosition;

  mat4 colorComponents;
  for (int i = 0; i < 3; ++i)
    colorComponents[i] = u_AverageColor[a_BlockIndex][2 * i + (v_VertexToCamera[i] > 0 ? 1 : 0)];
  colorComponents[3] = vec4(0);

  v_ColorComponents = colorComponents;
  gl_Position = u_ViewProjection * vec4(vertexPosition, 1);
}



#type fragment
#version 460 core

layout(location = 0) in vec3 v_LocalPosition;
layout(location = 1) in vec3 v_VertexToCamera;
layout(location = 2) in mat4 v_ColorComponents;

layout(location = 0) out vec4 o_Color;

void main()
{
  vec3 normal = normalize(cross(dFdx(v_LocalPosition), dFdy(v_LocalPosition)));

  float lightValue = (1 + normal.z) / 2;

  // Block faces can only be seen if surface normal component is facing camera
  vec4 colorStrengths;
  for (int i = 0; i < 3; ++i)
    colorStrengths[i] = v_VertexToCamera[i] > 0 == normal[i] > 0 ? abs(v_VertexToCamera[i] * normal[i]) : 0;
  colorStrengths.w = 0;

  float normalization = colorStrengths.x + colorStrengths.y + colorStrengths.z;
  vec4 baseColor = v_ColorComponents * colorStrengths / normalization;
  o_Color = vec4(vec3(lightValue), 1) * baseColor;

  // Debug - makes wireframes visible
  // o_Color = vec4(vec3(0), 1);
}