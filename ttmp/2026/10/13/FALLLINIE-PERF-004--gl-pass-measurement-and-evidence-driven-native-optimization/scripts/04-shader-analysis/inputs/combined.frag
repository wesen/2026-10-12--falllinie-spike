#version 300 es
precision highp float;
uniform sampler2D uC; uniform highp sampler2D uD; uniform vec2 uRes; uniform mat4 uIVP; uniform vec3 uCamP, uSunG, uPaper, uInk, uGrey;
uniform float uNear, uFar, uPitch, uInv, uTime, uSpeed, uFlash, uEW, uApx, uSunR; uniform vec2 uFogP; uniform vec2 uSunPx;
out vec4 o;
float lin(float d){ float z=d*2.-1.; return 2.*uNear*uFar/(uFar+uNear-z*(uFar-uNear)); }
float h1(float x){ return fract(sin(x*127.1)*43758.5453); }
float n1(float x){ float i=floor(x), f=fract(x); f=f*f*(3.-2.*f); return mix(h1(i),h1(i+1.),f); }
float hs(vec2 p){ return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453); }
float halftone(float t, vec2 fc){
  if(t<=0.01) return 0.;
  vec2 p=mat2(0.7071,-0.7071,0.7071,0.7071)*fc/uPitch; vec2 c=fract(p)-0.5;
  float r=sqrt(t)*0.66, aa=0.9/uPitch; return smoothstep(r+aa,r-aa,length(c));
}

uniform highp float uSkySamples[170];
float skyHash(float index){return uSkySamples[int(index)];}
float skyNoise(float az,float frequency,float offset){float count=floor(6.28318530718*frequency+.5);float x=(az+3.14159265359)/6.28318530718*count+offset;float i=floor(x),f=fract(x);f=f*f*(3.-2.*f);return mix(skyHash(mod(i,count)),skyHash(mod(i+1.,count)),f);}
void main(){
  vec2 fc=gl_FragCoord.xy, uv=fc/uRes, e=uEW/uRes;
  float d=texture(uD,uv).r;
  float iv=step(uv.x*0.35+(1.-uv.y)*0.65, uInv*1.25-0.12);
  vec3 Pp=mix(uPaper,uInk,iv), Kk=mix(uInk,uPaper,iv);
  vec3 col=Pp, dotc=uGrey; float black=0., grey=0., tone=0., sun=0.;
  if(d>=0.99999){
    vec4 w=uIVP*vec4(uv*2.-1.,1.,1.); vec3 dir=normalize(w.xyz/w.w-uCamP);
    float el=dir.y, az=atan(dir.x,dir.z);
    float r1=0.04+0.07*skyNoise(az,3.,3.)+0.03*skyNoise(az,11.,0.);
    float r2=0.012+0.04*skyNoise(az,6.,11.)+0.012*skyNoise(az,27.,0.);
    tone = el<r2 ? 0.46 : (el<r1 ? 0.2 : smoothstep(0.3,0.9,el)*0.16);
    black=max(black,1.-smoothstep(0.,uApx*1.3,abs(el-r1)));
    black=max(black,(1.-smoothstep(0.,uApx*1.3,abs(el-r2)))*step(el,r1));
    float sd=length(fc-uSunPx);
    sun=(1.-smoothstep(uSunR-1.,uSunR+1.,sd))*step(r1,el);
  } else {
    vec4 s=texture(uC,uv); float lum=s.r; black=s.g; grey=s.a; float id=s.b;
    float zc=lin(d); float edge=0.;
    if(zc<uFogP.y){
    float zl=lin(texture(uD,uv-vec2(e.x,0.)).r), zr=lin(texture(uD,uv+vec2(e.x,0.)).r), zu=lin(texture(uD,uv+vec2(0.,e.y)).r), zd=lin(texture(uD,uv-vec2(0.,e.y)).r);
    float lap=(abs(zl+zr-2.*zc)+abs(zu+zd-2.*zc))/zc;
    edge=smoothstep(0.018,0.045,lap);
    float il=texture(uC,uv-vec2(e.x,0.)).b, ir=texture(uC,uv+vec2(e.x,0.)).b, iu=texture(uC,uv+vec2(0.,e.y)).b, idn=texture(uC,uv-vec2(0.,e.y)).b;
    float dI=max(max(abs(il-id),abs(ir-id)),max(abs(iu-id),abs(idn-id)));
    edge=max(edge,step(0.002,dI));
    edge*=1.-smoothstep(uFogP.x,uFogP.y,zc);
    }
    black=max(black,edge);
    tone=clamp((0.86-lum)/0.72,0.,1.);
    dotc=mix(uGrey,Kk,smoothstep(0.36,0.12,lum));
  }
  float cov=halftone(tone,fc);
  col=mix(col,dotc,cov); col=mix(col,uGrey,clamp(grey,0.,1.)); col=mix(col,uGrey,sun); col=mix(col,Kk,clamp(black,0.,1.));
  if(uSpeed>0.01){
    vec2 q=(fc-uRes*0.5)/uRes.y; float a=atan(q.y,q.x), r=length(q);
    float k=(a+3.14159)/6.28318*150., id2=floor(k), hh=hs(vec2(id2,3.1));
    float seg=fract(r*1.4-uTime*(2.2+hh*3.)+hh*7.);
    float ln=step(0.84,hh)*step(0.58,seg)*smoothstep(0.34,0.6,r)*step(abs(fract(k)-0.5),0.1)*uSpeed;
    col=mix(col,Kk,ln);
  }
  col+=(hs(fc+fract(uTime*7.)*91.)-0.5)*0.028;
  col=mix(col,Kk,uFlash);
  o=vec4(col,1.);
}
