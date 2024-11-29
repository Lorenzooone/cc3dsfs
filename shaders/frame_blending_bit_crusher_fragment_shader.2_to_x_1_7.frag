uniform sampler2D Texture0;
uniform vec2 old_frame_offset;

const float inner_divisor = x;
const float bit_crusher = 255.0 / inner_divisor;
const float normalizer = inner_divisor * (255.0 / (256.0 - inner_divisor)) / 255.0;

void main() {
	gl_FragColor = texture2D(Texture0, gl_TexCoord[0].xy);
	gl_FragColor = clamp(floor(gl_FragColor * bit_crusher) * normalizer, 0.0, 1.0);
	vec4 OldFragColor = texture2D(Texture0, gl_TexCoord[0].xy + old_frame_offset);
	OldFragColor = clamp(floor(OldFragColor * bit_crusher) * normalizer, 0.0, 1.0);
	gl_FragColor = (gl_FragColor + OldFragColor) / 2.0;
}
