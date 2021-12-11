#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;

uniform vec3 u_ChunkPosition;
uniform mat4 u_ViewProjection;

void main()
{
  gl_Position = u_ViewProjection * vec4(u_ChunkPosition + a_Position, 1.0);
}



#type fragment
#version 330 core

layout(location = 0) out vec4 color;

void main()
{
  color = vec4(0.2f, 0.52f, 0.18f, 1.0f);
}