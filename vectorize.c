#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GW 32
#define SVGDIM 4
int gw,gh;
char g[GW*17];
int linenum=1;

int unhex(char*src,int lgt)
{
  int i;
  int sum=0;
  for(i=0;i<lgt;i++)
  {
    int a=src[i]|32;
    sum<<=4;
    if(a>='0' && a<='9') sum+=a-'0'; else
    if(a>='a' && a<='f') sum+=a-'a'+10; else
    {
      fprintf(stderr,"illegal char at %d here: %s\n",i,src);
      exit(1);
    }
  }
//  fprintf(stderr,"unhex:%x\n",sum);
  return sum;
}

void drawgrid()
{
  int i,j;
  for(i=0;i<gh;i++)
  {
    for(j=0;j<gw;j++)
      putchar(".#**"[g[i*GW+j]]);
    putchar('\n');
  }
}

int unpackgrid(char*src)
{
  int i,j;
  gw=(strlen(src)*4)/gh;
  for(i=0;i<gh;i++)
  {
    int a=unhex(src+(i*(gw/4)),gw/4);
    for(j=gw-1;j>=0;j--)
    {
      g[i*GW+j] = a&1;
      a>>=1;
    }
  }
}

int leftholes(int x,int y)
{
  while(x>=0 && g[y*GW+x]) x--;
  if(x<0) return 0;
  while(x>=0 && !g[y*GW+x]) x--;
  if(x<0) return 0;
  return 1;
}

int dir,steps;

void changedir(int d)
{
  if(dir!=d)
  {
    if(dir==0) printf("h%d",steps*SVGDIM); else
    if(dir==1) printf("h%d",-steps*SVGDIM); else
    if(dir==3) printf("v%d",steps*SVGDIM); else
    if(dir==2) printf("v%d",-steps*SVGDIM);
    steps=0;
  }
  dir=d;
  if(d==-1) printf("z");
}

void startglyph(int index,int width) // todo also note wcwidth()==0
{
  printf("<glyph unicode=\"&#%d;\" ",index);
  if(width!=8) printf("horiz-adv=\"%d\"",width*SVGDIM);
  printf("><path d=\"");
}

void endglyph()
{
  printf("\" /></glyph>\n");
}

void setpos(int x,int y)
{
  printf("M%d %d",x*SVGDIM,y*SVGDIM);
  steps=0;
  dir=-2;
}

void right()
{
  changedir(0);
  steps++;
}

void left()
{
  changedir(1);
  steps++;
}

void up()
{
  changedir(2);
  steps++;
}

void down()
{
  changedir(3);
  steps++;
}

void makepath(int x0,int y0)
{
  int x=x0,y=y0,c=0,returning=0;
  int prevdir;
  
  setpos(x,y);
  for(;;)
  {
    if(!returning)
    {
    if((x<GW-1) && g[y*GW+x] && !leftholes(x-1,y)) { x++; prevdir=2; right(); }
    else
    if((y<gh) && x>0 && g[y*GW+(x-1)] && !leftholes(x-1,y))
    {
      int i=x-1;
      while(i>=0 && g[y*GW+i]) { g[y*GW+i]|=2; i--; }
      y++; down(); prevdir=3;
    } else returning=1;
    } else
    {
      if(x==x0 && y==y0) break;
      
      if((x>0) && g[(y-1)*GW+x-1]) { x--; prevdir=0; left(); }
      else if(y>0 && g[(y-1)*GW+x]) { y--; prevdir=1; up(); }
      else if(g[y*GW+x] && (prevdir!=0)) { prevdir=2; x++; right(); }
      else
      {
        fprintf(stderr,"GOT STUCK!\n");
        exit(1);
      }
    }
    
/*    
    x

    if(y>0 && g[(y-1)*GW+(x-1)]) { x--; left(); }
    else if(y<=0) break;
    else { y--; up(); }
*/

  }
  changedir(-1);

//  drawgrid();

  for(y=0;y<gh;y++)
  for(x=0;x<gw;x++)
    if(g[y*GW+x]&2) { g[y*GW+x]=0; c++; }
  if(!c)
  {
    fprintf(stderr,"makepath fail!\n");
    exit(1);
  }
}

int vectorizepart()
{
  int y,x;
  for(y=0;y<gh;y++)
  for(x=0;x<gw;x++)
  {
    if(g[y*GW+x]==1)
    {
      makepath(x,y);
      return 1;
    }
  }
  return 0;
}

void makeheader(int is8,char*extraname)
{
printf(
  "<?xml version=\"1.0\" standalone=\"yes\"?>\n"
  "<svg version=\"1.1\" xmlns = 'http://www.w3.org/2000/svg'>\n"
  "<defs><font id=\"Unscii\" horiz-adv-x=\"32\">\n"
  "<font-face font-family=\"Unscii\" font-weight=\"medium\" font-style=\"normal\"\n"
  "units-per-em=\"%d\" cap-height=\"%d\" x-height=\"%d\" ascent=\"%d\" descent=\"%d\">\n"
  "<font-face-src><font-face-name name=\"Unscii %s\" /></font-face-src></font-face>\n",
  (is8?8:16)*SVGDIM,
  (is8?8:16)*SVGDIM,
  (is8?5:7)*SVGDIM,
  (is8?8:16)*SVGDIM,
  0,
  extraname
  );
}

void makefooter()
{
  printf("</font></defs></svg>\n");
}

int main(int argc,char**argv)
{
  char buf[128];
//  char*buf="0040:00007CC6C6C6DEDEDEDCC0C07C000000";
  int i=0;
  int glyphidx;
  gh=(argc==1 || argv[1][0]=='8')?8:16;
  if(argc>=2 && !strcmp(argv[2],"tall")) gh=16;
  makeheader((gh==8)?1:0, argc>2?argv[2]:"");
  for(;;)
  {
    if(feof(stdin)) break;
    fgets(buf,128,stdin);
    while(buf[i] && (buf[i]!=':')) i++;
    glyphidx=unhex(buf,i);
    if(glyphidx>=32)
    {
      unpackgrid(buf+i+1);
      startglyph(glyphidx,gw);
      if(!vectorizepart())
      {
        printf("M0 0");
      } else
      {
        while(vectorizepart());
      }
      endglyph();
    }
  }
  makefooter();
  return 0;
}
