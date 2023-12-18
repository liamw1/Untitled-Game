#type vertex
#version 460 core

const vec2 s_TexCoords[4] = vec2[4]( vec2(0.0f,	0.0f),
                                     vec2(1.0f, 0.0f),
                                     vec2(1.0f, 1.0f),
                                     vec2(0.0f, 1.0f) );

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};
layout(std140, binding = 3) uniform LOD
{
  vec3 u_Anchor;
  float u_TextureScaling;
  float u_NearPlaneDistance;
  float u_FarPlaneDistance;
};
float C = 2.0 / log(u_FarPlaneDistance / u_NearPlaneDistance);

layout(binding = 0) uniform sampler2DArray u_TextureArray;

layout(location = 0) in vec3 a_Position;
layout(location = 2) in ivec2 a_TextureIndices;
layout(location = 3) in vec2 a_TextureWeights;
layout(location = 4) in int a_QuadIndex;

layout(location = 0) out vec3 v_LocalWorldPosition;
layout(location = 1) out vec4 v_Color;

void main()
{
  v_LocalWorldPosition = a_Position;
  gl_Position = u_ViewProjection * vec4(u_Anchor + a_Position, 1.0f);

  // Applying logarithmic depth buffer
  // NOTE: This might be disorting normals far from the camera,
  //	   it may be better to give shader a modified camera instead
  // gl_Position.z = C * log(gl_Position.w / u_NearPlaneDistance) - 1; 
  // gl_Position.z *= gl_Position.w;

  vec2 texCoord = u_TextureScaling * s_TexCoords[a_QuadIndex];
  vec4 texture0 = a_TextureWeights[0] * texture(u_TextureArray, vec3(texCoord, a_TextureIndices[0]));
  vec4 texture1 = a_TextureWeights[1] * texture(u_TextureArray, vec3(texCoord, a_TextureIndices[1]));

  v_Color = texture0 + texture1;
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
  if (o_Color.a == 0)
	  discard;
}