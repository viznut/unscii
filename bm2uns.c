#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

// todo:
// - include faster mode (with binary search + comparison to 4x4 masks)
// - support 8x16 mask
// - support combining diacritics
// - support palette quantization
// - support different character subsets (unicode, cp437, ascii, petscii, etc)
// - optionalize sdl dependency (support only pnm then)
// - uns2bm

typedef struct
{
  uint8_t bm[8];
  int unicode;
  // todo: flags for char typeclass and subset
} charbm_t;

charbm_t bitmaps[] =
// 8x8 bitmap or each unscii char & sourcechar & variant
{
#include "bm2uns.i"
};

typedef int color_t;

typedef struct
{
  int sz;
  color_t colors[256];
} palette_t;

typedef struct
{
  int w,h;
  color_t*pixels;
} pixmap_t;

typedef struct
{
  int w,h;
  char*pixels;
} bitmap_t;

pixmap_t src;
palette_t palette;

#define MAX(a,b) ((a)>(b)?(a):(b))

int colorDiffRGB(uint8_t*c0,uint8_t*c1)
{
  return (c0[0]-c1[0])*(c0[0]-c1[0]) +
         (c0[1]-c1[1])*(c0[1]-c1[1]) +
         (c0[2]-c1[2])*(c0[2]-c1[2]);
}

float hueDiffRGB(uint8_t*c0,uint8_t*c1)
{
  float d0[3],d1[3];
  int i;
  float i0=c0[0]*3+c0[1]*6+c0[2];
  float i1=c1[0]*3+c1[1]*6+c1[2];
  if(i0==0 || i1==0) return 0;
  for(i=0;i<3;i++)
  {
    d0[i]=((double)c0[i])/i0;
    d1[i]=((double)c1[i])/i1;
  }
  return (d0[0]-d1[0])*(d0[0]-d1[0]) +
         (d0[1]-d1[1])*(d0[1]-d1[1]) +
         (d0[2]-d1[2])*(d0[2]-d1[2]);
}

void splitToRGB(color_t c,uint8_t*rgb)
{
  rgb[0]=(c>>16)&255;
  rgb[1]=(c>>8)&255;
  rgb[2]=c&255;
}

color_t fromRGB(uint8_t*rgb)
{
  return (rgb[0]<<16)|(rgb[1]<<8)|rgb[2];
}

int colorDiff(color_t c0,color_t c1)
{
  uint8_t rgb0[3],rgb1[3];
  if(c0==c1) return 0;
  if(c0==-1 || c1==-1) return 3*256*256;
  splitToRGB(c0,rgb0);
  splitToRGB(c1,rgb1);
  return colorDiffRGB(rgb0,rgb1);
}

int colorDiffWithHue(color_t c0,color_t c1)
{
  uint8_t rgb0[3],rgb1[3];
  if(c0==c1) return 0;
  if(c0==-1 || c1==-1) return 3*256*256;
  splitToRGB(c0,rgb0);
  splitToRGB(c1,rgb1);
  return colorDiffRGB(rgb0,rgb1) * (1.0 + hueDiffRGB(rgb0,rgb1));
}

int getBestGridOffset(int*boundaries,int num,int istep,int*bestscore_ret)
{
  int i0;
  int best=num/istep,where=num-1;
  for(i0=0;i0<istep;i0++)
  {
    int sum=0;
    int i;
    for(i=i0;i<num;i+=istep)
      sum+=boundaries[i];
    if(sum>best)
    {
      if(where!=0 || sum>(best*3)/2)
      {
        best=sum;
        where=i0;
      }
    }
  }
  *bestscore_ret=best;
  return where;
}

void guessGridFromBoundaries(int*boundaries,int num,int*i0_r,int*istep_r,
  int otherdim)
{
  int bs[17],bo[17];
  int beststep=8,bestscore=(num*otherdim)/32;
  int istep;
  for(istep=2;istep<=16;istep++)
  {
    int score;
    bo[istep]=getBestGridOffset(boundaries,num,istep,&score);
    score*=istep;
    bs[istep]=score;
//    fprintf(stderr,"for step %d best offset is %d (score %d)\n",
//      istep,bo[istep],bs[istep]);
    if(score > bestscore)
    {
      bestscore = score;
      beststep = istep;
    }
  }
  if(beststep!=8 && bestscore<(bs[8]*5)/4) beststep=8;
  
  *i0_r = (bo[beststep]+1)%beststep;
  *istep_r = beststep;
}

int guessGrid(pixmap_t*pm,int*xy_r,int*xycellsize_r)
{
  int*boundaries=alloca(MAX(pm->w,pm->h)*sizeof(int));
  int y,x,v;

  for(x=0;x<pm->w;x++) boundaries[x]=0;
  for(x=0;x<pm->w-1;x++)
    for(y=0;y<pm->h;y++)
      if(pm->pixels[y*pm->w+x]!=pm->pixels[y*pm->w+(x+1)])
         boundaries[x]++;
  guessGridFromBoundaries(boundaries,pm->w,xy_r+0,xycellsize_r+0,pm->h);

  for(y=0;y<pm->h;y++) boundaries[y]=0;
  for(y=0;y<pm->h-1;y++)
    for(x=0;x<pm->w;x++)
      if(pm->pixels[y*pm->w+x]!=pm->pixels[(y+1)*pm->w+x])
         boundaries[y]++;
  guessGridFromBoundaries(boundaries,pm->h,xy_r+1,xycellsize_r+1,pm->w);
}

// PuTTY is used for reference because it's common, close enough to vga
// and provides sensible color mixing with extended xterm colors.
color_t ansicolors[16]={
  0x000000,
  0xbb0000,
  0x00bb00,
  0xbb5500,
  0x0000bb,
  0xbb00bb,
  0x00bbbb,
  0xbbbbbb,
  0x555555,
  0xff5555,
  0x55ff55,
  0xffff55,
  0x5555ff,
  0xff55ff,
  0x55ffff,
  0xffffff
};

void makeDefaultPalette()
{
  // low 16 colors as in putty
  // extended colors as in xterm, vte and putty
  const uint8_t cubelevels[6] = { 0,95,135,175,215,255 };
  int i,j,k;
  memcpy(palette.colors,ansicolors,16*sizeof(color_t));
//  makeAnsiPalette();
  palette.sz=256;
  for(k=0;k<6;k++)
  for(j=0;j<6;j++)
  for(i=0;i<6;i++)
    palette.colors[16+i+j*6+k*36] = 
      (cubelevels[k]<<16)|
      (cubelevels[j]<<8)|
      cubelevels[i];
  for(i=0;i<24;i++)
    palette.colors[232+i] = 0x10101 * (8+10*i);
}

void get2NearestPaletteColorsBelow(color_t c,int*ret,int sz)
{
  int i,mindiff=colorDiff(c,palette.colors[0]),where=0,
  mindiff2=mindiff,where2=where;
  for(i=1;i<sz;i++)
  {
    int d=colorDiff(c,palette.colors[i]);
    if(d<mindiff)
    {
      mindiff2=mindiff;
      where2=where;
      mindiff=d;
      where=i;
    } else if(d<mindiff2)
    {
      mindiff2=d;
      where2=i;
    }
  }
  ret[0]=where;
  ret[1]=where2;
}

void get2NearestPaletteColors(color_t c,int*ret)
{
  get2NearestPaletteColorsBelow(c,ret,palette.sz);
}

int getNearestPaletteColor(color_t c)
{
  int r[2];
  get2NearestPaletteColors(c,r);
  return r[0];
}

int getNearestPaletteColorBelow(color_t c,int sz)
{
  int r[2];
  get2NearestPaletteColorsBelow(c,r,sz);
  return r[0];
}

void getContrastColor(color_t targetcol,color_t nearcol,uint8_t*rgb,
  int mul)
{
  int i;
  uint8_t trg[3],near[3];
  splitToRGB(targetcol,trg);
  splitToRGB(nearcol,near);
  for(i=0;i<3;i++)
  {
    int a=near[i] + (trg[i]-near[i])*mul;
    if(a<0) a=0;
    if(a>255) a=255;
    rgb[i]=a;
  }
}

void getRasterMixForColor(color_t c,int*colors_r,float*ratio_r)
{
  int candidates[8];
  double canddiff[8];
  int d0,d1;
  float ratio;
  get2NearestPaletteColors(c,candidates);
  if(c==palette.colors[candidates[0]]) // || candidates[0]==candidates[1])
  {
    colors_r[0]=candidates[0];
    colors_r[1]=-1;
    *ratio_r=0;
    return;
  }

  uint8_t rgb[3];
  
  get2NearestPaletteColorsBelow(c,candidates+2,MAX(palette.sz,232));
  
  int i,j,k;
  int best=6*256*256;
  
  for(i=0;i<4;i++)
  {
    canddiff[i] = sqrt(colorDiff(c,palette.colors[candidates[i]]));
  }

  for(i=0;i<4;i++)
  {
   uint8_t c0[3],c1[3],mix[3];
   splitToRGB(palette.colors[candidates[i]],c0);
   for(j=0;j<i;j++)
   {
    double ratio = canddiff[i] / (canddiff[i]+canddiff[j]);
    splitToRGB(palette.colors[candidates[j]],c1);

    for(k=0;k<3;k++)
      mix[k] = c0[k]*(1.0-ratio) + c1[k]*ratio;

    int d=colorDiff(c,fromRGB(mix));
    if(d<best)
    {
      best=d;
      colors_r[0]=candidates[i];
      colors_r[1]=candidates[j];
      *ratio_r=ratio;
    }
   }
  }
  
  if(*ratio_r>0.5)
  {
    int tmp=colors_r[0];
    *ratio_r=1.0-ratio;
    colors_r[0]=colors_r[1];
    colors_r[1]=tmp;
  }
  
  if(*ratio_r<0) *ratio_r=0;
  if(*ratio_r>1) *ratio_r=1;
  
//  if(colors_r[0]==colors_r[1]) { *ratio_r=0; colors_r[1]=-1; }
  
//  fprintf(stderr,"combine %d & %d with ratio %f\n",
//    colors_r[0],colors_r[1],*ratio_r);
}

void getMidColor(pixmap_t*bm,uint8_t*midrgb)
{
  int midr=0,midg=0,midb=0;
  int i;
  for(i=0;i<bm->w*bm->h;i++)
  {
    int col=bm->pixels[i];
    midr+=(col>>16)&255;
    midg+=(col>>8)&255;
    midb+=col&255;
  }
  midr/=(bm->w*bm->h); if(midr>255) midr=255;
  midg/=(bm->w*bm->h); if(midg>255) midg=255;
  midb/=(bm->w*bm->h); if(midb>255) midb=255;
  midrgb[0]=midr;
  midrgb[1]=midg;
  midrgb[2]=midb;
}

int quantizeTo2(pixmap_t*bm,color_t*cols_r)
{
  int i;
  int c0r=0,c0g=0,c0b=0,c0freq=0;
  int c1r=0,c1g=0,c1b=0,c1freq=0;  
  uint8_t midrgb[3];
  getMidColor(bm,midrgb);
  for(i=0;i<bm->w*bm->h;i++)
  {
    int r,g,b,selbg=0;
    int col=bm->pixels[i];
    r=(col>>16)&255;
    g=(col>>8)&255;
    b=col&255;
    if(r<=midrgb[0]) selbg++;
    if(g<=midrgb[1]) selbg++;
    if(b<=midrgb[2]) selbg++;
    if(selbg>=2)
    {
      c1r+=r;
      c1g+=g;
      c1b+=b;
      c1freq++;
    } else
    {
      c0r+=r;
      c0g+=g;
      c0b+=b;
      c0freq++;
    }
  }
  if(c0freq==0) cols_r[0]=-1; else
  {
    c0r/=c0freq;
    c0g/=c0freq;
    c0b/=c0freq;
    cols_r[0] = (c0r<<16)|(c0g<<8)|c0b;  
  }
  
  if(c1freq==0) cols_r[1]=-1; else
  {
    c1r/=c1freq;
    c1g/=c1freq;
    c1b/=c1freq;
    cols_r[1] = (c1r<<16)|(c1g<<8)|c1b;
  }
  if(c0freq<c1freq)
  {
    int tmp=cols_r[0];
    cols_r[0]=cols_r[1];
    cols_r[1]=tmp;
  }
}

bitmap_t*allocBitmap(int w,int h)
{
  bitmap_t*bm;
  bm->w=w;
  bm->h=h;
  bm->pixels=malloc(w*h*sizeof(char));
  return bm;
}

void getBitmap(pixmap_t*pm,color_t fg,color_t bg,bitmap_t*bm)
{
  int x,y,i;
  bm->w=pm->w;
  bm->h=pm->h;
  if(bm->pixels==NULL) bm->pixels=malloc(bm->w*bm->h*sizeof(char));
  for(i=0;i<bm->h*bm->w;i++)
  {
    int bit;
    color_t c=pm->pixels[i];
    int d=colorDiff(c,bg);
    if(!d) bit=0; else bit=(d<colorDiff(c,fg))?0:1;
    bm->pixels[i]=bit;
  }
}

uint8_t ditherpatterns[10][8] =
{
  0,0,0,0,0,0,0,0,				// 0/8
  0x22,0x00,0x88,0x00,0x22,0x00,0x88,0x00,	// 1/8
  0x22,0x88,0x22,0x88,0x22,0x88,0x22,0x88,	// 2/8
  0xaa,0x44,0xaa,0x11,0xaa,0x44,0xaa,0x11,	// 3/8 (diagonal croshatch)
  0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55,	// 4/8
  0x66,0xcc,0x66,0xee,0x66,0xcc,0x66,0xee,
  0xee,0x77,0xee,0x77,0xee,0x77,0xee,0x77,
  0xee,0xff,0x77,0xff,0xee,0xff,0x77,0xff,
  255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,
};

void getDitheredBitmap(pixmap_t*pm,color_t fg,color_t bg,bitmap_t*bm)
{
  int x,y,i,j;
  bm->w=pm->w;
  bm->h=pm->h;
  if(bm->pixels==NULL) bm->pixels=malloc(bm->w*bm->h*sizeof(char));
  for(j=0;j<bm->h;j++)
  for(i=0;i<bm->w;i++)
  {
    int bit;
    color_t c=pm->pixels[i];
    float d0=sqrt(colorDiff(c,bg));
    float d1=sqrt(colorDiff(c,fg));
    int pattern=trunc(d0*8)/(d0+d1);
    if(pattern<0) pattern=0;
    if(pattern>7) pattern=7;

    bm->pixels[j*bm->w+i]= //pattern<(rand()&7)?0:1;
      (ditherpatterns[pattern][j&7]>>(7-(i&7)))&1;
  }
}

int getNearestBraille(bitmap_t*b)
{
  int i,j;
  int grid[2*4];
  for(i=0;i<2*4;i++) grid[i]=0;

  for(j=0;j<8;j++)
  for(i=0;i<8;i++)
  {
    int u=i/4;
    int v=j/2;
    if(b->pixels[j*8+i])grid[2*v+u]++;
  }
  
  return 0x2800+
    (grid[0]>=4?1:0) +
    (grid[1]>=4?8:0) +
    (grid[2]>=4?2:0) +
    (grid[3]>=4?16:0) +
    (grid[4]>=4?4:0) +
    (grid[5]>=4?32:0) +
    (grid[6]>=4?64:0) +
    (grid[7]>=4?128:0);
}

int scalePixels(pixmap_t*bm,pixmap_t*bm_r,int w1,int h1)
{
}

int countBits(uint8_t a)
{
  int c=0;
  while(a) { c+=a&1; a>>=1; }
  return c;
}

int findUnicode(bitmap_t*bm,charbm_t*tab,int sz,
  int flags) // tässä 0=bg, 1=fg, 2=nocare
{
  int i,j;
  uint8_t tofind[8];
  if(bm->w!=8 || bm->h!=8) return -1;
  

  for(j=0;j<8;j++)
  {
    int a=0;
    for(i=0;i<8;i++)
    {
      int b=bm->pixels[j*8+i];
      //if(b==2) ... reserved for combining diacritics support
      a=(a<<1)|b;
    }
    tofind[j]=a;
  }

  int best=8*8,where;
  for(j=0;j<sz;j++)
  {
    int diff=0;
    for(i=0;i<8;i++)
    {
      diff += countBits(tab[j].bm[i]^tofind[i]);
    }
    if(diff<best)
    {
      if(!diff) return tab[j].unicode;
      best=diff;
      where=tab[j].unicode;
    }
    if(flags && (63-diff<best))
    {
      if(diff==63) return ~tab[j].unicode;
      best=63-diff;
      where=~tab[j].unicode;
    }
  }
  
  return where; //tab[where].unicode;
}

uint16_t unsciiShades[33] =
{
  0x00A0,	// 0	0/32
  0xE190,       // 1	1/32
  0xE191,       // 2	1/16
  0xE192,       // 3	3/32
  0xE192,       // 4	1/8
  0xE193,       // 5	5/32
  0xE193,       // 6	3/16	
  0x2591,       // 7	7/32
  0x2591,	// 8	1/4
  0x2591,       // 9	9/32
  0x2591,       // 10	10/32
  0x2591,       // 11	11/32
  0x2591,       // 12	12/32
  0x2592,       // 13	13/32
  0x2592,       // 14	14/32
  0x2592,       // 15	15/32
  0x2592,       // 16	16/32
  0x2592,       // 17	17/32
  0x2592,       // 18	18/32
  0x2592,       // 19	19/32
  0x2593,       // 20
  0x2593,       // 21
  0x2593,       // 22
  0x2593,       // 23
  0x2593,       // 24
  0x2593,       // 25
  0x2593,       // 26
  0xE194,       // 27
  0xE194,       // 28
  0xE195,       // 29
  0xE195,       // 30
  0xE196,       // 31
  0x2588        // 32
};

void printUTF8(int unicode)
{
  if(unicode<0x80) putchar(unicode);
  else if(unicode<0x800)
  {
    putchar(0xc0|(unicode>>6));
    putchar(0x80|(unicode&63));
  }
  else if(unicode<0x10000)
  {
    putchar(0xe0|(unicode>>12));
    putchar(0x80|((unicode>>6)&63));
    putchar(0x80|(unicode&63));
  }
  else if(unicode<0x20000)
  {
    putchar(0xf0|(unicode>>18));
    putchar(0x80|((unicode>>12)&63));
    putchar(0x80|((unicode>>6)&63));
    putchar(0x80|(unicode&63));
  } else
  {
    putchar(0xf8|(unicode>>24));
    putchar(0x80|((unicode>>18)&63));
    putchar(0x80|((unicode>>12)&63));
    putchar(0x80|((unicode>>6)&63));
    putchar(0x80|(unicode&63));
  }
}

int currfg=-1,currbg=-1,currattr=0;

void printColors(int fg,int bg)
{
  if(fg==-1) fg=currfg;
  if(bg==-1) bg=currbg;

  if(fg==currfg && bg==currbg) return;

  printf("\033[");

  if(fg!=currfg)
  {
    if(fg>=8)
      printf("38;5;%d",fg);
    else
      printf("%d",fg+30);
    if(bg!=currbg) printf(";");
  }
  if(bg!=currbg)
  {
    if(bg>=8)
      printf("48;5;%d",bg);
    else
      printf("%d",bg+40);
  }
  putchar('m');
  currbg=bg;
  currfg=fg;
}

void grabCell(pixmap_t*src,pixmap_t*c,int x0,int y0,int w,int h)
{
  int i,j;
  for(j=0;j<c->h;j++)
  {
    color_t*s0=src->pixels+x0+(y0+(j*h)/c->h)*src->w;

    // todo bilinear maybe
    for(i=0;i<c->w;i++)
    {
      c->pixels[j*c->w+i] = s0[(i*w)/c->w];
    }
  }
}

int palnum=232;
int algo=0;

void doConversion(int cellw,int cellh,int offx,int offy)
{
  int usecolor=1;
  pixmap_t cell;
  bitmap_t bm;
  int i,j,u,v;

  cell.w=8;
  cell.h=8; // tai 16
  cell.pixels=malloc(cell.w*cell.h*sizeof(color_t));

  bm.w=cell.w;
  bm.h=cell.h;
  bm.pixels=malloc(cell.w*cell.h*sizeof(char));

  if(palnum<8) usecolor=0;

  makeDefaultPalette();
  if(usecolor)
  {
    if(palnum==232)
    {
      int i;
      for(i=0;i<16;i++) palette.colors[i]=-1;
    } else palette.sz=palnum;
    printf("\033[0m");
  } else makeDefaultPalette();

  for(j=offy;j<=src.h-cellh;j+=cellh)
  {
    int cx=0;
    for(i=offx;i<=src.w-cellw;i+=cellw)
    {
      uint8_t rgb[3];
      grabCell(&src,&cell,i,j,cellw,cellh);

      int cols[2];
      if(usecolor) getMidColor(&cell,rgb);
      float r;
      int block;

     {
        int c0,c1;
     
        if(usecolor)
        {
          quantizeTo2(&cell,cols);

          c0=getNearestPaletteColor(cols[0]);
          c1=getNearestPaletteColor(cols[1]);
        } else
        {
          if(palnum==0)
          {
            c0=15;
            c1=0;
          } else
          {
            c0=0;
            c1=15;
          }
        }

        int ditherthis=0;

        if(algo!=0 && (c0==c1))
        {
          uint8_t rgb0[3],rgb1[3];

          {
            getContrastColor(cols[1],palette.colors[c0],rgb1,2);
            c1=getNearestPaletteColor(fromRGB(rgb1));
          }
          ditherthis=1;
        }
//        else
        {
          int unic;
          if(ditherthis || algo==2)
            getDitheredBitmap(&cell,palette.colors[c0],palette.colors[c1],&bm);
          else
            getBitmap(&cell,palette.colors[c0],palette.colors[c1],&bm);

          unic=findUnicode(&bm,bitmaps,sizeof(bitmaps)/sizeof(charbm_t),
            usecolor?1:0);
            
          if(algo!=0 && (unic==0x20 || unic==0x25bb || unic==-0x20
            || unic==-0x25bb))
          {
            getDitheredBitmap(&cell,palette.colors[c0],palette.colors[c1],&bm);
            unic=findUnicode(&bm,bitmaps,sizeof(bitmaps)/sizeof(charbm_t),
              usecolor?1:0);
          }
            
          if(unic>=0)
          {
            if(usecolor) printColors(c0,c1);
            printUTF8(unic);
          } else
          {
            if(usecolor) printColors(c1,c0);
            printUTF8(~unic);
          }
        }
    }
    }
    if(usecolor) printf("\033[0m");
    putchar('\n');
    currfg=currbg=-1;
  }
}

/** ** **/

void getSDLPixel(SDL_Surface*s,int x,int y,Uint8*r,Uint8*g,Uint8*b)
{
  void*pp = (void*)((Uint8*)s->pixels) + s->pitch*y;
  if(s->format->BytesPerPixel==1)
    SDL_GetRGB(((Uint8*)pp)[x],s->format,r,g,b);
  else if(s->format->BytesPerPixel==2)
    SDL_GetRGB(((Uint16*)pp)[x],s->format,r,g,b);
  else if(s->format->BytesPerPixel==4)
    SDL_GetRGB(((Uint32*)pp)[x],s->format,r,g,b);
  else
    SDL_GetRGB(*((Uint32*)(pp+x*s->format->BytesPerPixel)),s->format,r,g,b);
}

void loadImage(char*fn)
{
  int i,j;
  SDL_Surface*s0;
  SDL_Init(SDL_INIT_VIDEO);
  s0 = IMG_Load(fn);
  if(!s0)
  {
    // loadimage_internal for extra formats and pnmless operation
    return;
  }
  src.w=s0->w;
  src.h=s0->h;
  if(s0->format->BitsPerPixel==32)
  {
    src.pixels=s0->pixels;
    return;
  }
  else
  {
//    fprintf(stderr,"converting %d-bpp %d-opp image, size %d x %d\n",
//      s0->format->BitsPerPixel,s0->format->BytesPerPixel,
//      src.w,src.h);
    src.pixels=malloc(src.w*src.h*sizeof(color_t));
    
    for(j=0;j<src.h;j++)
    for(i=0;i<src.w;i++)
    {
      Uint8 r,g,b;
      getSDLPixel(s0,i,j,&r,&g,&b);
      src.pixels[j*src.w+i]=(r<<16)|(g<<8)|b;
    }
  }
}

/** ** **/

int main(int argc,char**argv)
{
  int i,j;
  int cellwh[2]={8,8};
  int celloffset[2]={0,0};
  int fitwidth=0;

  src.h=0;
  if(argc<2)
  {
    fprintf(stderr,
      "converts a bitmap image to unscii art.\n"
      "usage: %s [options] filename\n"
      "where options include:\n"
      "-c<w>x<h>[+<x>+<y>] set cell size and offset.\n"
      "  default: 8x8+0+0\n"
      "  empty parameter: guess from content\n"
//      "-f<n> with to the width of n columns (empty: use window width)\n"
      "-p<n> select palette:\n"
      "  0 = white on black, 1 = black on white\n"
      "  8 = 8-color ansi, 16 = 16-color pc-ansi\n"
      "  256 = xterm default 256-color\n"
      "  232 = 256 without ansi-16 portion (default)\n"
//      "      t = truecolor (output rgb colors without palette)\n"
//      "      q<n> = quantize from content (n = max number of colors)\n"
//      "      pf<filename> = read from file\n"
//      "      ps<filename> = read from file and use as subset\n"
      "-a<n> select algorithm:\n"
      "  0 = no dither (default)\n"
      "  1 = dither otherwise-monochrome cells\n"
      "  2 = dither all cells\n"
//      "-C<n> limit character set:\n
//      "      unscii (default), unicode, cp437, latin1, ascii, petscii\n"
//	"      gfxonly (only use graphics-oriented blocks)\n"
    ,argv[0]);
    return 1;
  }
  for(i=1;i<argc;i++)
  {
    if(argv[i][0]=='-' && argv[i][1])
    {
      if(argv[i][1]=='a') algo=atoi(argv[i]+2);
      if(argv[i][1]=='c')
      {
        if(argv[i][2])
        {
          sscanf(argv[i]+2,"%dx%d+%d+%d",cellwh,cellwh+1,celloffset,celloffset+1);
        } else cellwh[1]=0;
      }
      if(argv[i][1]=='p')
      {
        palnum=atoi(argv[i]+2);
      }
      if(argv[i][1]=='f')
      {
        fitwidth=atoi(argv[i]+2);
        if(fitwidth<=0)
        {
          char*c=getenv("COLUMNS");
          if(c) fitwidth=atoi(c);
          if(fitwidth<=0) fitwidth=80;
        }
      }
    } else
    {
      if(!src.h)
      {
        loadImage(argv[i]);
        if(!src.h)
        {
          fprintf(stderr,"can't load %s\n",argv[i]);
          return 1;
        }
      } else
      {
        fprintf(stderr,"ignored %s\n",argv[i]);
      }
    }
  }

  // guess dimensions if not given on commandline  
  if(!cellwh[1])
  {
    guessGrid(&src,celloffset,cellwh);
    fprintf(stderr,"guessed grid dimensions: cell size %dx%d, offset %d:%d\n",
      cellwh[0],cellwh[1],celloffset[0],celloffset[1]);
  }

  if(fitwidth>0)
  {
    float aspect = (float)cellwh[1] / (float)cellwh[0];
    cellwh[0] = src.w / fitwidth;
    cellwh[1] = cellwh[0] * aspect;
  }

  doConversion(cellwh[0],cellwh[1],celloffset[0],celloffset[1]);
 
  return 0;
}
