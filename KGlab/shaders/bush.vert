#version 120
varying vec3 fragPos;
varying vec3 normal;
varying vec2 TexCoord;

void main()
{
    fragPos = vec3(gl_ModelViewMatrix * gl_Vertex);
    normal = gl_NormalMatrix * gl_Normal;
    TexCoord = gl_MultiTexCoord0.xy;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}