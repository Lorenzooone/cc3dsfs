uniform sampler2D Texture0;

void main() {
	gl_FragColor = texture2D(Texture0, gl_TexCoord[0].xy);
};
