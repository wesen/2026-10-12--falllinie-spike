#version 300 es
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNor; layout(location=2) in float aAlb; layout(location=3) in vec2 aPaint; layout(location=4) in vec2 aUR;
uniform mat4 uVP, uM; uniform mat3 uN;
out vec3 vW; out vec3 vN; out float vA; out vec2 vP; out vec2 vU;
void main(){ vec4 w=uM*vec4(aPos,1.); vW=w.xyz; vN=uN*aNor; vA=aAlb; vP=aPaint; vU=aUR; gl_Position=uVP*w; }
