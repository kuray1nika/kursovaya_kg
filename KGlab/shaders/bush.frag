#version 120
varying vec3 fragPos;
varying vec3 normal;
varying vec2 TexCoord;

uniform sampler2D diffuse_tex;
uniform sampler2D normal_tex;
uniform sampler2D specular_tex;
uniform vec3 light_pos_v;
uniform vec3 view_pos;

void main()
{
    vec4 diff_color = texture2D(diffuse_tex, TexCoord);
    vec3 normal_map = texture2D(normal_tex, TexCoord).rgb;
    vec3 spec_map = texture2D(specular_tex, TexCoord).rgb;
    
    vec3 N = normalize(normal_map * 2.0 - 1.0);
    vec3 L = normalize(light_pos_v - fragPos);
    vec3 V = normalize(view_pos - fragPos);
    vec3 H = normalize(L + V);
    
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    
    vec3 ambient = vec3(0.1);
    vec3 result = ambient * diff_color.rgb + diff * diff_color.rgb + spec * spec_map;
    
    gl_FragColor = vec4(clamp(result, 0.0, 1.0), diff_color.a);
}