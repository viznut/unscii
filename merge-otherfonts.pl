#!/usr/bin/env perl

use Text::CharWidth qw(mbwidth mbswidth mblen);

sub detectmargins()
{
      # detect margins
      for($j=0;$j<length($bits);$j++)
      {
        if(substr($bits,$j,1) eq '1') { $top=int($j/$wdt); $j=666; }
      }
      for($j=length($bits)-1;$j>=0;$j--)
      {
        if(substr($bits,$j,1) eq '1') { $bottom=int((length($bits)-$j)/$wdt); $j=-1; }
      }
      for($j=0;$j<$wdt;$j++)
      {
        for($k=$j;$k<16*$wdt+$j;$k+=$wdt)
        {
          if(substr($bits,$k,1) eq '1') { $left=$j; $j=$wdt; $k=666; }
        }
      }
      for($j=$wdt-1;$j>=0;$j--)
      {
        for($k=$j;$k<16*$wdt;$k+=$wdt)
        {
          if(substr($bits,$k,1) eq '1') { $right=($wdt-1-$j); $j=0; $k=666; }
        }
      }
}

sub fixf()
{
  return '0'.$_[0] if substr($_[0],4,1) eq ':';
  return $_[0];
}

open(F,'unscii-16.hex');
$fglyph=<F>;
open(G,'unifont.hex');
$gglyph=<G>;
open(H,'fsex-adapted.hex');
$hglyph=<H>;
$fglyph=&fixf($fglyph);
$gglyph=&fixf($gglyph);
$hglyph=&fixf($hglyph);

for($i=0;$i<0x20000;$i++)
{
  $hex=sprintf('%05X',$i);

  if(substr($fglyph,0,length($hex)) eq $hex)
  {
    $fg=$fglyph;
    $fglyph=<F>;
    $fglyph=&fixf($fglyph);
  } else { $fg=''; }

  if(substr($gglyph,0,length($hex)) eq $hex)
  {
    $gg=$gglyph;
    $gglyph=<G>;
    $gglyph=&fixf($gglyph);
  } else { $gg=''; }

  if(substr($hglyph,0,length($hex)) eq $hex)
  {
    $hg=$hglyph;
    $hglyph=<H>;
    $hglyph=&fixf($hglyph);
  } else { $hg=''; }
  
  #print "$hex\n F:$fg($fglyph) G:$gg($gglyph) H:$hg($hglyph)\n";

  if($fg)
  {
    print $fg;
  } elsif($hg || $gg)
  {
    $rightwdt=8*mbwidth(chr(hex($hex)));

    if($hg)
    {
      $g = $hg;
      $src = 'F';
    } 
    elsif($gg)
    {
      $g = $gg;
      $src = 'U';
    }

    $bitmap = substr($g,6);
    chop $bitmap;
    $wdt=int(length($bitmap)/4);
    if($rightwdt<0) { $rightwdt=$wdt; }
    elsif($rightwdt==0) { $rightwdt=8; }
    if(($wdt!=$rightwdt) && ($src eq 'F'))
    {
      $abitmap = substr($gg,6);
      chop $abitmap;
      $awdt=int(length($abitmap)/4);
      if($awdt==$rightwdt)
      {
        $wdt=$awdt;
        $g=$gg;
        $bitmap=$abitmap;
        $src = 'U';
      }
    }

    if($wdt==16 && $rightwdt==16)
    {
      print $g;
    } else
    {
      $right=$left=$top=$bottom=0;
      $bits=unpack('B*',pack('H*',$bitmap));
      #print "bits: $bits\n";

      &detectmargins();

      #print "right width $rightwdt; margins: top $top bottom $bottom left $left right $right\n";

      if($wdt==8 && $rightwdt==16)
      {
        $bits =~ s/0/00/g;
        $bits =~ s/1/11/g;
        #print "doubled: $bits\n";
        $wdt=16;
        if($left==0 && $right==1)
        {
          $bits='0'.substr($bits,0,length($bits)-1);
        }
        elsif($left==1 && $right==0)
        {
          $bits=substr($bits,1).'0';
        }
      } elsif($wdt==16 && $rightwdt==8)
      {
        #print "shrinking...\n";
        $b='';
        for($j=0;$j<length($bits);$j+=2)
        {
          if(substr($bits,0,4) eq '0110' &&
             ($j%$wdt<=($wdt-4)))
          {
            $b.='0';
          } else
          {
            $b.=substr($bits,$j,1)|substr($bits,$j+1,1);
          }
        }
        $bits=$b;
        $wdt=8;
        &detectmargins();
      }
      
      # special style adjustments for 8x16 chars in unifont
      if(($wdt==8) && ($src eq 'U'))
      {
        # if first line is clear, move 1 pixel upwards
        if($top>=1)
        {
          $bits=substr($bits,8).'00000000';
          $top--; $bottom++;
        }
        
        # left-leaning instead of right-leaning
        if($left==1 && $right==0)
        {
          $bits=substr($bits,1).'0';
          $left=0; $right=1;
        }
        
        # shrink small letters by one pixel if possible
        if(($top==5) && ($bottom<=3))
        {
          for($j=$top+1;$j<16-$bottom;$j++)
          {
            if(substr($bits,$j*8,8) eq substr($bits,$j*8+8,8))
            {
              $bits='00000000'.substr($bits,0,$j*8).substr($bits,($j+1)*8);
              $j=666;
              break;
            }
          }
        }

        # thicken if possible without blur
        if(($bits =~ /010/) && !($bits =~ /101/))
        {
          if($left>=1)
          {
            #print "thicken left\n";
            $bits =~ s/010/X10/g;
            $bits =~ s/011/X11/g;
            $bits =~ tr/X/1/;
          } elsif($right>=2)
          {
            #print "thicken right\n";
            $bits =~ s/010/01X/g;
            $bits =~ s/110/11X/g;
            $bits =~ tr/X/1/;
          }
        }
      }

      #for($j=0;$j<$wdt*16;$j+=$wdt)
      #{
      #  $z=substr($bits,$j,$wdt)."\n";
      #  $z=~tr/01/\.\#/;
      #  print $z;
      #}
      $bitmap=unpack('H*',pack('B*',$bits));
      $bitmap =~ tr/a-f/A-F/;

    print "$hex:$bitmap\n";
  }
  } else
  {
    #print "$hex:MISSING\n";
  }
}
