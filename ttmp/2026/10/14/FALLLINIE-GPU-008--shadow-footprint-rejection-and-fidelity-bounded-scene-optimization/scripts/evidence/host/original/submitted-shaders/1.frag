#version 300 es
precision highp float;
in vec3 vW; in vec3 vN; in float vA; in vec2 vP; in vec2 vU;
uniform int uKind; uniform float uMat, uAlbO, uXray; uniform vec3 uCam, uL; uniform vec2 uFog; uniform vec4 uSeg[6]; uniform sampler2D uGlyph;
out vec4 o;
float rayseg(vec3 P, vec3 L, vec3 A, vec3 B){
  vec3 v=B-A, w=P-A; float b=dot(L,v), c=dot(v,v), d=dot(L,w), e=dot(v,w), D=c-b*b;
  float tc = D<1e-5 ? clamp(e/c,0.,1.) : clamp((e-b*d)/D,0.,1.);
  vec3 Q=A+v*tc; float sc=max(dot(Q-P,L),0.); return length(P+L*sc-Q);
}
void main(){
  vec3 n=normalize(vN);
  float alb = uAlbO>=0. ? uAlbO : vA;
  float lum = alb*(0.6+0.5*max(dot(n,uL),0.));
  float blk=0., gry=0., dist=length(vW-uCam);
  if(uKind==2){ if(dot(vU,vU)>1.) discard; lum=0.97; }
  if(uKind==0||uKind==3){
    float sh=1.;
    for(int i=0;i<3;i++){ vec4 a=uSeg[i*2], b=uSeg[i*2+1]; sh=min(sh, smoothstep(a.w*0.7, a.w*1.2, rayseg(vW,uL,a.xyz,b.xyz))); }
    lum*=mix(0.34,1.,sh);
  }
  if(uKind==0 && uXray>0.5){
    // x-ray: a 2 m survey grid in lateral u and downhill z, heavier every 10 m and at every 110 m cell edge
    vec2 g=vec2(vU.x,vW.z)/2., fg=fwidth(g);
    vec2 l=1.-smoothstep(0.4,1.2,abs(fract(g-0.5)-0.5)/max(fg,vec2(1e-4)));
    vec2 G=vec2(vU.x,vW.z)/10., fG=fwidth(G), L2=1.-smoothstep(0.5,1.6,abs(fract(G-0.5)-0.5)/max(fG,vec2(1e-4)));
    float fade=(1.-smoothstep(50.,170.,dist))*(1.-smoothstep(0.35,0.8,max(fg.x,fg.y)));
    gry=max(gry,max(l.x,l.y)*0.55*fade); gry=max(gry,max(L2.x,L2.y)*fade);
    float ce=vW.z/110., fc=fwidth(ce); blk=max(blk,(1.-smoothstep(0.5,2.,abs(fract(ce-0.5)-0.5)/max(fc,1e-4)))*(1.-smoothstep(120.,300.,dist)));
    lum=mix(1.,lum,0.45);
  }
  if(uKind==0){
    float c=vU.y/0.55, fw=fwidth(c);
    float ln=1.-smoothstep(0.35,1.1,abs(fract(c-0.5)-0.5)/max(fw,1e-4));
    ln*=(1.-smoothstep(0.18,0.5,fw))*(1.-smoothstep(80.,190.,dist));
    gry=max(gry,ln*0.9);
    blk=max(blk,smoothstep(0.3,0.75,vP.x));
    if(vP.y>0.3){ float st=step(0.5,fract((vW.x+vW.z)*0.42)); blk=max(blk,st*smoothstep(0.3,0.7,vP.y)); }
    float ni=floor(vW.z/250.+0.5), lz=vW.z-ni*250.;
    if(ni>=1. && abs(lz)<9.){
      float gw=4.4, lx=-vU.x, gi=floor((lx+2.*gw)/gw);
      if(gi>=0. && gi<4.){
        float dg=mod(floor(ni*250./pow(10.,3.-gi)+0.001),10.);
        vec2 g=vec2(fract((lx+2.*gw)/gw),(lz+9.)/18.);
        blk=max(blk,texture(uGlyph,vec2((dg+g.x)/10.,1.-g.y)).a);
      }
    }
  }
  float f=smoothstep(uFog.x,uFog.y,dist);
  lum=mix(lum,1.,f); blk*=1.-f; gry*=1.-f;
  o=vec4(clamp(lum,0.,1.),blk,uMat/255.,gry);
}
