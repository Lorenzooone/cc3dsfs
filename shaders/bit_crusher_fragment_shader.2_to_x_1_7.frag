uniform sampler2D Texture0;

const float inner_divisor = x;
const float bit_crusher = 255.0 / inner_divisor;
const float normalizer = inner_divisor * (255.0 / (256.0 - inner_divisor)) / 255.0;

void main() {
	gl_FragColor = clamp(floor(texture2D(Texture0, gl_TexCoord[0].xy) * bit_crusher) * normalizer, 0.0, 1.0);
};
