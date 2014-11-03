#include "color.h"

static const uint32_t ABITS = 4;
static const uint32_t HSCALE = 256;

rgb_t hsv2rgb(hsv_t c){
  rgb_t r;
  uint32_t m;
  int32_t H,X,ih,is,iv;
  const uint32_t k1=255 << ABITS;
  const int32_t k2=HSCALE << ABITS;
  const int32_t k3=1<<(ABITS-1);

  // set chroma and min component value m
  //chroma = ( c.v * c.s )/k1;
  //m = c.v - chroma;
  m = ((uint32_t)c.v*(k1 - (uint32_t )c.s ))/k1;

  // chroma  == 0 <-> c.s == 0 --> m=c.v
  if (c.s == 0) {
      r.blue = ( r.green = ( r.red = c.v >> ABITS ));
  } else {
    ih=(int32_t)c.h;
    is=(int32_t)c.s;
    iv=(int32_t)c.v;

    H = (6*ih)/k2;
    X = ((iv*is)/k2)*(k2-abs(6*ih- 2*(H>>1)*k2 - k2)) ;

    // removing additional bits --> unit8
    X=((X+iv*(k1 - is))/k1 + k3) >>ABITS;
    m=m >> ABITS;

    // ( chroma + m ) --> c.v ;
    switch (H) {
        case 0:
          r.red = c.v >> ABITS ;
          r.green = X;
          r.blue = m ;
          break;
        case 1:
          r.red = X;
          r.green = c.v >> ABITS;
          r.blue = m ;
          break;
        case 2:
          r.red = m ;
          r.green = c.v >> ABITS;
          r.blue = X;
          break;
        case 3:
          r.red = m ;
          r.green = X;
          r.blue = c.v >> ABITS;
          break;
        case 4:
          r.red = X;
          r.green = m ;
          r.blue = c.v >> ABITS;
          break;
        case 5:
          r.red = c.v >> ABITS;
          r.green = m ;
          r.blue = X;
          break;
    }
  }
  return r;
}
