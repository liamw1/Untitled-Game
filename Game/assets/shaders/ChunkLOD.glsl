#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 2) in int a_QuadIndex;

const vec2 s_TexCoords[4] = vec2[4]( vec2(0.0f,	0.0f),
									 vec2(1.0f, 0.0f),
									 vec2(1.0f, 1.0f),
									 vec2(0.0f, 1.0f) );

uniform float u_TextureScaling;
uniform vec3 u_LODPosition;
uniform mat4 u_ViewProjection;
uniform float u_NearPlaneDistance;
uniform float u_FarPlaneDistance;

float C = 2.0 / log(u_FarPlaneDistance / u_NearPlaneDistance);

out vec3 v_TexCoord;
out vec3 v_LocalWorldPosition;

void main()
{
  v_TexCoord = vec3(u_TextureScaling * s_TexCoords[a_QuadIndex], float(1));
  v_LocalWorldPosition = a_Position;
  gl_Position = u_ViewProjection * vec4(u_LODPosition + a_Position, 1.0f);

  // Applying logarithmic depth buffer
  // NOTE: This might be disorting normals far from the camera,
  //	   it may be better to give shader a modified camera instead
  gl_Position.z = C * log(gl_Position.w / u_NearPlaneDistance) - 1; 
  gl_Position.z *= gl_Position.w;
}



#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 v_TexCoord;
in vec3 v_LocalWorldPosition;

uniform sampler2DArray u_TextureArray;

void main()
{
  vec3 normal = -normalize(cross(dFdy(v_LocalWorldPosition), dFdx(v_LocalWorldPosition)));
  float lightValue = (1.0 + normal.z) / 2;

  color = vec4(vec3(lightValue), 1.0f) * texture(u_TextureArray, v_TexCoord);
  if (color.a == 0)
	discard;
}