#version 430 core

uniform float u_near;
uniform float u_far;

out vec4 out_fragColor;

float linearizeDepth(float depth) {
	return (2.0 * u_near) / (u_far + u_near - depth * (u_far - u_near));
}

void main() {
	float z = gl_FragCoord.z; // Non-linear depth value
	float linearDepth = linearizeDepth(z);
	// store the fragment depth and also its linearized value - useful for debugging
	out_fragColor = vec4(z, linearDepth, 0.0, 1.0);
}

