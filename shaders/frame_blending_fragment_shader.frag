uniform sampler2D Texture0;
uniform vec2 old_frame_offset;

void main() {
	gl_FragColor = texture2D(Texture0, gl_TexCoord[0].xy);
	vec4 OldFragColor = texture2D(Texture0, gl_TexCoord[0].xy + old_frame_offset);
	gl_FragColor = (gl_FragColor + OldFragColor) / 2.0;
}
