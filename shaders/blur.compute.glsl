#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform readonly image2D inputImage;
layout(rgba32f, binding = 1) uniform writeonly image2D outputImage;

uniform int u_radius;          // blur radius, e.g. 3, 5, 8
uniform float u_sigma;         // gaussian sigma, e.g. 2.0
uniform float u_blurStrength;  // 0.0 = no blur, 1.0 = full blur

float gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

void main()
{
    ivec2 texSize = imageSize(inputImage);
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

    if (gid.x >= texSize.x || gid.y >= texSize.y) {
        return;
    }

    vec4 originalColor = imageLoad(inputImage, gid);

    int radius = max(u_radius, 0);

    if (radius == 0) {
        imageStore(outputImage, gid, originalColor);
        return;
    }

    vec4 blurredColor = vec4(0.0);
    float totalWeight = 0.0;

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            ivec2 sampleCoord = gid + ivec2(x, y);

            // clamp to texture borders
            sampleCoord = clamp(sampleCoord, ivec2(0), texSize - ivec2(1));

            float wx = gaussian(float(x), u_sigma);
            float wy = gaussian(float(y), u_sigma);
            float weight = wx * wy;

            blurredColor += imageLoad(inputImage, sampleCoord) * weight;
            totalWeight += weight;
        }
    }

    blurredColor /= totalWeight;

    // Mix original and blurred image
    vec4 finalColor = mix(originalColor, blurredColor, clamp(u_blurStrength, 0.0, 1.0));

    imageStore(outputImage, gid, finalColor);
}