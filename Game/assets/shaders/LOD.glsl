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

  for (int axis = 0; axis < 3; ++axis)
    v_ColorComponents[axis] = u_AverageColor[a_BlockIndex][2 * axis + (v_VertexToCamera[axis] > 0 ? 1 : 0)];
  v_ColorComponents[3] = vec4(0);

  gl_Position = u_ViewProjection * vec4(vertexPosition, 1);
}



#type fragment
#version 460 core

const float c_Pi = radians(180);

layout(location = 0) in vec3 v_LocalPosition;
layout(location = 1) in vec3 v_VertexToCamera;
layout(location = 2) in mat4 v_ColorComponents;

layout(location = 0) out vec4 o_Color;

void main()
{
  vec3 normal = normalize(cross(dFdx(v_LocalPosition), dFdy(v_LocalPosition)));

  // Crude approximation of block-based ambient occlusion based on surface normal (can probably do much better than this)
  float lightValue = (3 + abs(cos(c_Pi * normal.z))) / 4;

  // Block faces can only be seen if surface normal component is facing camera
  vec4 colorStrengths;
  for (int axis = 0; axis < 3; ++axis)
    colorStrengths[axis] = v_VertexToCamera[axis] > 0 == normal[axis] > 0 ? abs(v_VertexToCamera[axis] * normal[axis]) : 0;
  colorStrengths.w = 0;

  // Here it turns out normalization can be NaN in wireframe mode, so it's important to keep that in mind when doing comparisons.
  // Remember, a NaN will always return false when compared used <,>,<=,>=,== and always true with !=.
  float normalization = colorStrengths.x + colorStrengths.y + colorStrengths.z;
  vec4 baseColor = normalization > 0 ? v_ColorComponents * colorStrengths / normalization : vec4(vec3(0), 1);
  o_Color = vec4(vec3(lightValue), 1) * baseColor;
}