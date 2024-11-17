uniform sampler2D Texture0;
uniform vec2 old_frame_offset;
const float inner_divisor = x;
const float inner_divisor_greater = inner_divisor * 2.0;
const float bit_crusher = 255.0 / inner_divisor;
const float bit_crusher_greater = 255.0 / inner_divisor_greater;

vec4 get_nth_bit_component(vec4 colors) {
	return (floor(colors * bit_crusher) * inner_divisor) - (floor(colors * bit_crusher_greater) * inner_divisor_greater);
}


void main() {
	gl_FragColor = texture2D(Texture0, gl_TexCoord[0].xy);
	vec4 OldFragColor = texture2D(Texture0, gl_TexCoord[0].xy + old_frame_offset);
	vec4 actual_nth_bit_component = get_nth_bit_component(gl_FragColor);
	vec4 old_nth_bit_component = get_nth_bit_component(OldFragColor);
	gl_FragColor = clamp((floor(gl_FragColor * 255.0) - actual_nth_bit_component + ((actual_nth_bit_component + old_nth_bit_component) / 2.0)) / 255.0, 0.0, 1.0);
};
