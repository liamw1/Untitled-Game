#type vertex
#version 450 core

const vec2 s_LocalPositions[4] = vec2[4]( vec2(-1.0f, -1.0f),
                                          vec2( 1.0f, -1.0f),
                                          vec2( 1.0f,  1.0f),
                                          vec2(-1.0f,  1.0f) );

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in float a_Thickness;
layout(location = 3) in float a_Fade;
layout(location = 4) in int a_QuadIndex;
layout(location = 5) in int a_EntityID;

layout(location = 0) out vec2 v_LocalPosition;
layout(location = 1) out vec4 v_Color;
layout(location = 2) out flat float v_Thickness;
layout(location = 3) out flat float v_Fade;
layout(location = 4) out flat int v_EntityID;

void main()
{
  v_LocalPosition = s_LocalPositions[a_QuadIndex];
  v_Color = a_Color;
  v_Thickness = a_Thickness;
  v_Fade = a_Fade;
  v_EntityID = a_EntityID;

  gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}



#type fragment
#version 450 core

layout(location = 0) in vec2 v_LocalPosition;
layout(location = 1) in vec4 v_Color;
layout(location = 2) in flat float v_Thickness;
layout(location = 3) in flat float v_Fade;
layout(location = 4) in flat int v_EntityID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

void main()
{
  // Calculate distance and fill circle with white
  float distance = 1.0f - length(v_LocalPosition);
  float alpha = smoothstep(0.0f, v_Fade, distance);
  alpha *= smoothstep(v_Thickness + v_Fade, v_Thickness, distance);

  if (alpha == 0.0f)
    discard;

  o_Color = v_Color;
  o_Color.a *= alpha;
  o_EntityID = v_EntityID;
}