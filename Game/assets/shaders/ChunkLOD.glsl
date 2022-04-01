#type vertex
#version 450 core

const vec2 s_TexCoords[4] = vec2[4]( vec2(0.0f,	0.0f),
									                   vec2(1.0f, 0.0f),
									                   vec2(1.0f, 1.0f),
									                   vec2(0.0f, 1.0f) );

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};
layout(std140, binding = 2) uniform LOD
{
	vec3 u_Anchor;
  float u_TextureScaling;
  float u_NearPlaneDistance;
  float u_FarPlaneDistance;
};
float C = 2.0 / log(u_FarPlaneDistance / u_NearPlaneDistance);

layout(location = 0) in vec3 a_Position;
layout(location = 2) in int a_TextureIndex;
layout(location = 3) in int a_QuadIndex;

layout(location = 0) out flat int v_TextureIndex;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 2) out vec3 v_LocalWorldPosition;

void main()
{
  v_TextureIndex = a_TextureIndex; 
  v_TexCoord = u_TextureScaling * s_TexCoords[a_QuadIndex];

  v_LocalWorldPosition = a_Position;
  gl_Position = u_ViewProjection * vec4(u_Anchor + a_Position, 1.0f);

  // Applying logarithmic depth buffer
  // NOTE: This might be disorting normals far from the camera,
  //	     it may be better to give shader a modified camera instead
  gl_Position.z = C * log(gl_Position.w / u_NearPlaneDistance) - 1; 
  gl_Position.z *= gl_Position.w;
}



#type fragment
#version 450 core

layout(binding = 0) uniform sampler2DArray u_TextureArray;

layout(location = 0) in flat int v_TextureIndex;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) in vec3 v_LocalWorldPosition;

layout(location = 0) out vec4 o_Color;

void main()
{
  vec3 normal = -normalize(cross(dFdy(v_LocalWorldPosition), dFdx(v_LocalWorldPosition)));
  float lightValue = (1.0 + normal.z) / 2;

  o_Color = vec4(vec3(lightValue), 1.0f) * texture(u_TextureArray, vec3(v_TexCoord, v_TextureIndex));
  if (o_Color.a == 0)
	  discard;
}