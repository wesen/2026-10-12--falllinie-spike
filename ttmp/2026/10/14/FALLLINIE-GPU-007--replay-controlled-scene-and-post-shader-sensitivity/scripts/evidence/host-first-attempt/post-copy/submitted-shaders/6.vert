#version 300 es
precision highp float;
layout(location=0) in vec2 aPosition;layout(location=1) in vec2 aUV;
uniform vec2 uOutputRes;out vec2 vUV;
void main(){vUV=aUV;gl_Position=vec4(aPosition/uOutputRes*2.-1.,0.,1.);}