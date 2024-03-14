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

void main()
{
  vec3 vertexPosition = u_AnchorPosition[gl_DrawID].xyz + a_Position;
  vec3 toVertex = vertexPosition - u_CameraPosition;

  float norm = abs(toVertex.x) + abs(toVertex.y) + abs(toVertex.z);
  vec4 xColor = abs(toVertex.x / norm) * u_AverageColor[a_BlockIndex][toVertex.x < 0 ? 1 : 0];
  vec4 yColor = abs(toVertex.y / norm) * u_AverageColor[a_BlockIndex][toVertex.y < 0 ? 3 : 2];
  vec4 zColor = abs(toVertex.z / norm) * u_AverageColor[a_BlockIndex][toVertex.z < 0 ? 5 : 4];

  v_Color = xColor + yColor + zColor;
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