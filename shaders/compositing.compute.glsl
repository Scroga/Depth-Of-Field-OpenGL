#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform writeonly image2D u_outputImage;
layout(rgba32f, binding = 1) uniform readonly image2D u_diffuse;
layout(rgba32f, binding = 2) uniform readonly image2D u_shadows;

void main() {
				ivec2 texSize = imageSize(u_diffuse);
				ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

				if (gid.x >= texSize.x || gid.y >= texSize.y) {
								return;
				}

				vec4 color = imageLoad(u_diffuse, gid);
				float shadow = imageLoad(u_shadows, gid).x;

				color = vec4(color.rgb * min(1.0, shadow + 0.2), color.a);

				imageStore(u_outputImage, gid, color);
}
