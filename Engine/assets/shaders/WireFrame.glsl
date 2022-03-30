#type vertex
#version 450 core

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;

layout(location = 0) out vec4 v_Color;

void main()
{
  v_Color = a_Color;
  gl_Position = u_ViewProjection * vec4(a_Position, 1.0f);
}



#type fragment
#version 450 core

layout(location = 0) in vec4 v_Color;

layout(location = 0) out vec4 o_Color;

void main()
{
  o_Color = v_Color;
}