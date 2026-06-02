#version 120

varying vec3 fragPos;
varying vec3 normal_v;
varying vec2 TexCoord;

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    fragPos = vec3(gl_ModelViewMatrix * gl_Vertex);
    normal_v = normalize(gl_NormalMatrix * gl_Normal);
    TexCoord = gl_MultiTexCoord0.xy;
}