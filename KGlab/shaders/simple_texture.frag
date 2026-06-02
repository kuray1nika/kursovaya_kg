#version 120

varying vec2 TexCoord;
uniform sampler2D tex;

void main()
{
    vec4 color = texture2D(tex, TexCoord);
    if (color.a < 0.1) {
        discard;
    }
    
    gl_FragColor = color;
}