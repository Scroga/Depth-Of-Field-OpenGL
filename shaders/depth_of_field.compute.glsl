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

uniform int u_blurRadius;
uniform float u_gaussianSigma;

uniform bool u_debug;

float linearizeDepth(float depth) {
				float z = depth * 2.0 - 1.0;
				return (2.0 * u_near * u_far) / (u_far + u_near - z * (u_far - u_near));
}

float gaussian(float x) {
				return exp(-(x * x) / (2.0 * u_gaussianSigma * u_gaussianSigma));
}

void main() {
				ivec2 texSize = imageSize(inputImage);
				ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

				if (gid.x >= texSize.x || gid.y >= texSize.y) {
								return;
				}

				vec3 imageColor = imageLoad(inputImage, gid).xyz;

				if (u_blurRadius == 0) {
								imageStore(outputImage, gid, vec4(imageColor, 1.0));
								return;
				}

				// Compute blur intensity 
				float viewDist = clamp(u_distance / u_far, 0.0f, 1.0f);
				vec2 uv = (vec2(gid) + vec2(0.5)) / vec2(texSize);
				float rawDepth = texture(u_depthTexture, uv).r;
				float linearDepth = linearizeDepth(rawDepth);
				float depth = (linearDepth - u_near) / (u_far - u_near);
				depth = clamp(depth, 0.0, 1.0);

				float dist = abs(depth - clamp(viewDist, u_near, u_far));
				float blurIntensity = smoothstep(u_radius, u_radius + u_smoothness, dist);

				// Debug
				if (u_debug) {
								vec3 blue = imageColor * vec3(0.3, 0.3, 1.0);
								vec3 outputColor = mix(imageColor, blue, blurIntensity);
								imageStore(outputImage, gid, vec4(outputColor, 1.0));
								return;
				}

				// Apply blur
				vec3 blurredColor = vec3(0.0);
				float totalWeight = 0.0;

				for (int y = -u_blurRadius; y <= u_blurRadius; ++y) {
        for (int x = -u_blurRadius; x <= u_blurRadius; ++x) {
            ivec2 sampleCoord = gid + ivec2(x, y);

            sampleCoord = clamp(sampleCoord, ivec2(0), texSize - ivec2(1));

            float wx = gaussian(float(x));
            float wy = gaussian(float(y));
            float weight = wx * wy;

            blurredColor += imageLoad(inputImage, sampleCoord).xyz * weight;
            totalWeight += weight;
        }
    }

    blurredColor /= totalWeight;

				vec3 outputColor = mix(imageColor, blurredColor, blurIntensity);
				imageStore(outputImage, gid, vec4(outputColor, 1.0));
}