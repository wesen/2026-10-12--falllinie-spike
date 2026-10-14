#version 300 es
precision highp float;
uniform sampler2D uC; uniform highp sampler2D uD; uniform vec2 uOutputRes; uniform int uMode; out vec4 o;
void main(){vec2 uv=gl_FragCoord.xy/uOutputRes;vec4 c=texture(uC,uv);float x=0.;
if(uMode==0){o=vec4(c.rgb,1.);return;}
if(uMode==1)x=c.r;else if(uMode==2)x=c.g;else if(uMode==3)x=c.b*255./32.;else if(uMode==4)x=c.a;
else{float d=texture(uD,uv).r;float z=d*2.-1.;float linear=2.*.1*900./(900.+.1-z*(900.-.1));x=linear/150.;}
o=vec4(vec3(x),1.);}