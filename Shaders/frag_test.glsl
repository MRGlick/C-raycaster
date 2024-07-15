in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;
uniform float alpha;


void main(void)
{

    vec4 tex_col = texture2D(tex, texCoord);

	fragColor.rgb = tex_col.rgb;
    fragColor.r = alpha;
    fragColor.a = tex_col.a;
}
