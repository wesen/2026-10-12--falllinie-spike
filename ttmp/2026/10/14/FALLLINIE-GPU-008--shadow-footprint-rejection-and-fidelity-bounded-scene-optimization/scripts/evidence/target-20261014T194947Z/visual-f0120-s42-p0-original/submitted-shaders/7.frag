#version 300 es
precision highp float;
uniform highp sampler2D uFont;uniform vec3 uInk,uPaper;in vec2 vUV;out vec4 o;
void main(){if(vUV.x< -1.5){o=vec4(uInk,1.);return;}if(vUV.x<0.){o=vec4(uPaper,.92);return;}float alpha=texture(uFont,vUV).a;o=vec4(uInk,alpha);}