uniform sampler2D Texture0;
uniform float targetContrast;
uniform float targetBrightness;
uniform float targetLuminance;
uniform float targetGamma;
uniform float displayGamma;
uniform mat3 color_corr_mat;
uniform mat3 saturation_mat;

void main() {
	gl_FragColor = texture2D(Texture0, gl_TexCoord[0].xy);
	vec3 colors_to_edit = gl_FragColor.rgb;
	vec3 tatget_brightness_vec = vec3(targetBrightness);
	vec3 tatget_gamma_vec = vec3(targetGamma);
	vec3 display_gamma_vec = vec3(displayGamma);

	colors_to_edit += tatget_brightness_vec;
	colors_to_edit = pow(colors_to_edit, tatget_gamma_vec);

	colors_to_edit *= targetLuminance;
	colors_to_edit = clamp(colors_to_edit, 0.0, 1.0);

	colors_to_edit = color_corr_mat * colors_to_edit;
	colors_to_edit = saturation_mat * colors_to_edit;

	colors_to_edit = clamp(colors_to_edit, 0.0, 1.0);

	colors_to_edit *= targetContrast;
	colors_to_edit = pow(colors_to_edit, display_gamma_vec);

	colors_to_edit = clamp(colors_to_edit, 0.0, 1.0);
	gl_FragColor.rgb = colors_to_edit;
}
