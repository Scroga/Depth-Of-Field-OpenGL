#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform readonly image2D inputImage;
layout(rgba32f, binding = 1) uniform writeonly image2D outputImage;

uniform sampler2D u_depthTexture;

uniform float u_distance;
uniform float u_radius;
uniform float u_smoothness;

uniform float u_near;
uniform float u_far;


float linearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * u_near * u_far) / (u_far + u_near - z * (u_far - u_near));
}

void main() {
	ivec2 texSize = imageSize(inputImage);
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

	if (gid.x >= texSize.x || gid.y >= texSize.y) {
		return;
	}
		vec3 imageColor = imageLoad(inputImage, gid).xyz;
		
		vec2 uv = (vec2(gid) + vec2(0.5)) / vec2(texSize);
		float rawDepth = texture(u_depthTexture, uv).r;
		float depth = linearizeDepth(rawDepth) / u_far;

		float dist = abs(depth - clamp(u_distance, u_near, u_far));
		float intensity = smoothstep(u_radius, u_radius + u_smoothness, dist);

		vec3 outputColor = mix(imageColor, vec3(0.1, 0.2, 0.3), intensity);

	imageStore(outputImage, gid, vec4(outputColor, 1.0));
}