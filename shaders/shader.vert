#version 450

layout(binding = 0) uniform CameraUBO {
  mat4 model;
  mat4 view;
  mat4 projection;
} u_Camera3D;

// struct PointLight {
//   vec4 position; // ignore w
//   vec4 color;    // w is intensity
// };
// layout(binding = 0) uniform LightsUBO {
//   vec3 globalLightDirection;
//   vec4 globalLightColor; // w is intensity
//   PointLight pointLights[16];
// } u_Lights;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
  gl_Position = u_Camera3D.projection * u_Camera3D.view * u_Camera3D.model * vec4(position, 1.0);
  // gl_Position = vec4(position, 1.0);

  fragColor = vec3(1.0, 1.0, 1.0);
  fragTexCoord = inTexCoord;
}