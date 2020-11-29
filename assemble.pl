#!/usr/bin/env perl

use Encode;
# use Text::CharWidth;

%bitmaps8x8={};
%bitmaps8x16={};
%bitmaps16x16={};
%settings={};

$xtndpointer=0xe19c;
$xtnd2pointer=0xe852;
$xtnd3pointer=0xec7e;

$unsconvchars='';
$uniconvchars='';

$lowestchar=$highestchar=32;

sub dumpconvtab
{
  open(CT,">uns2uni.tr");
  print CT "$unsconvchars\n$uniconvchars";
  close(CT);
}

sub encutf8
{
  my $c=$_[0];
  return chr($c) if($c<0x80);
  return chr(0xc0|($c>>6)).chr(0x80|($c&63)) if ($c<0x800);
  return chr(0xe0|($c>>12)).chr(0x80|(($c>>6)&63)).
    chr(0x80|($c&63)) if($c<0x10000);
  return chr(0xf0|($c>>18)).chr(0x80|(($c>>12)&63)).chr(0x80|(($c>>6)&63)).
         chr(0x80|($c&63));
}

sub dumpfont
{
  $do8x8=0;
  $do8x16=0;
  foreach(split(',',$settings{'fontsizes'}))
  {
    $do8x8=1 if($_ eq '8x8') ;
    $do8x16=1 if($_ eq '8x16');
  }
  $p='';
  if($patch) { $p='-'.$patch; }
  open(F8,">unscii-8$p.hex") if($do8x8);
  open(F16,">unscii-16$p.hex") if($do8x16);

  print F8 "00000:00000000000000000000000000000000\n" if($do8x8);
  print F16 "00000:00000000000000000000000000000000\n" if($do8x16);

  %keycoll={};
  foreach(keys %bitmaps8x8)
  {
    if(/^[0-9A-F]+$/)
    {
      $numeric = hex($_);
      if($numeric>0) { $keycoll{$numeric}=1; }
    }
  }
  foreach(keys %bitmaps8x16)
  {
    if(/^[0-9A-F]+$/)
    {
      $numeric = hex($_);
      if($numeric>0) { $keycoll{$numeric}=1; }
    }
  }
  foreach(keys %bitmaps16x16)
  {
    if(/^[0-9A-F]+$/)
    {
      $numeric = hex($_);
      if($numeric>0) { $keycoll{$numeric}=1; }
    }
  }

  @keys={};
  foreach(keys %keycoll)
  {
    push(@keys,$_) if(/^[0-9]+$/);
  }

  $last=31;
  foreach(sort { $a - $b } @keys)
  {
    if($_ < 0x20000)
    {
      if($_ != ($last+1))
      {
#        printf("We have hole %04X .. %04X i.e. %s\n",$last+1,$_-1,
#          chr($last+1));
      }
      $hex=sprintf('%04X',$_);
      #print "U+$hex lgt=".length($bitmaps8x16{$hex})."\n";
      $hex8='';
      $hex16='';
      $hex1616='';
      foreach(split('',$bitmaps8x8{$hex}))
      {
        $hex8.=sprintf("%02X",ord($_));
      }
      foreach(split('',$bitmaps8x16{$hex}))
      {
        $hex16.=sprintf("%02X",ord($_));
      }
      foreach(split('',$bitmaps16x16{$hex}))
      {
        $hex1616.=sprintf("%02X",ord($_));
      }
      if($hex1616 ne '') { $hex16=$hex1616; }
      $hex=sprintf('%05X',$_);
      print F8 "$hex:$hex8\n" if($do8x8 && $hex8);
      print F16 "$hex:$hex16\n" if($do8x16 && $hex16);

#      foreach(split('',$bitmaps8x16{$hex}))
#      {
#        #$_=unpack('B8',$_);
#        #tr/01/\.\#/;
#        #print "$_ ";
#      }
#      #print "\n";
    
      $last=$_;
    }
  }
  close(F8);
  close(F16);
}

sub doublebytes
{
  my $r='';
  foreach(split('',$_[0])) { $r.=$_.$_; }
  return $r;
}

sub orbits
{
  my $r='';
  for(my $i=0;$i<length($_[0]);$i++)
  {
    $r .= chr(ord(substr($_[0],$i,1))|ord(substr($_[1],$i,1)));
  }
  return $r;
}

sub reversebitorder
{
  return pack('B8',unpack('b8',$_[0]));
}

sub dblw
{
  $_=unpack('B8',$_[0]);
  s/0/00/g;
  s/1/11/g;
  return pack('B16',$_[0]);
}

sub equateforconv
{
  my($uns,$uni)=@_;
  #print STDERR "equating: $uni <=> $uns\n";
  if(length($uni)==5 && hex($uni)>=0x10000 &&
     length($uns)==4 && hex($uns)>=0xE000 && hex($uns)<=0xF8FF) {
    $uniconvchars.=encutf8(hex($uni));
    $unsconvchars.=encutf8(hex($uns));
  }
}

sub readin
{
  my $filename = ($_[0] eq 'PATCH')?"font-$patch.txt":$_[0];
  return if($filename eq 'font-.txt');
  
  open(my $F,$filename) || die "File not found: $filename";
  print "Assembling ".$_[0]." ...\n";
  my $linenum=0;
  while(<$F>)
  {
    $linenum++;
    @p=split('\s',$_);
#    print "$p[0]...\n";
    if($p[0] eq 'INCLUDE')
    {
      &readin($p[1]);
    }
    elsif($p[0] eq 'SET')
    {
      $settings{$p[1]}=$p[2];
    }
    elsif(substr($p[0],0,2) eq 'U+')
    {
      $charname=substr($p[0],2);
      if($charname eq 'XTND')
      {
        $charname=sprintf('%04X',$xtndpointer);
        $xtndpointer++;
        $title=substr($_,7);
        #print "U+$charname : $title";
      }
      if($charname eq 'XTND2')
      {
        $charname=sprintf('%04X',$xtnd2pointer);
        $xtnd2pointer++;
        $title=substr($_,7);
        #print "U+$charname : $title";
      }
      if($charname eq 'XTND3')
      {
        $charname=sprintf('%04X',$xtnd3pointer);
        $xtnd3pointer++;
        $title=substr($_,7);
        #print "U+$charname : $title";
      }
#      print "Char $charname begins ...\n";
      if($bitline>=1 && $bitline!=$height)
      {
        print "Bitmap not $height pixels high! <$linenum \@ $filename\n";
        exit(1);
      }
      $bitline=0;
      $bitmap='';
      if($p[1] eq '=')
      {
        $comb=$p[2];
        $i=4;
        while(($#p>=$i) && ($p[$i-1] eq '+'))
        {
          $comb.='+'.$p[$i];
          $i+=2;
        }
        @parts=split('\+',$comb);
        $b8="\0\0\0\0\0\0\0\0";
        $b16=$b8.$b8;
        $b1616=$nulls=$b16.$b16;
        foreach(@parts)
        {
          if(substr($_,0,1) eq "'")
          {
            $_=sprintf("%04X",ord(decode('UTF-8',substr($_,1))));
          }
          if($bitmaps8x8{$_} eq '')
          {
            print "Glyph for $_ not defined! $linenum \@ $filename\n";
            exit(1);
          }
          $b8=&orbits($b8,$bitmaps8x8{$_});
          $b16=&orbits($b16,$bitmaps8x16{$_});
          $b1616=&orbits($b1616,$bitmaps16x16{$_});
        }
        if($#parts==0) { &equateforconv($charname,$parts[0]);
                         &equateforconv($parts[0],$charname); }
        $bitmaps8x8{$charname} = $b8;
        $bitmaps8x16{$charname} = $b16;
        $bitmaps16x16{$charname} = $b1616 if($b1616 ne $nulls);
      }
      elsif($p[1] eq 'DBLW')
      {
        if(!$bitmaps8x16{$p[2]})
        {
          print STDERR "Can't find bitmap $p[2] for dblw source! $linenum \@ $filename\n";
          exit(1);
        }
        $bitmaps8x8{$charname} = $bitmaps8x8{$p[2]};
        $b16='';
        $b16.=&dblw($_) foreach(split('',$bitmaps8x16{$p[2]}));
        $bitmaps16x16{$charname} = $b16;
      }
      elsif($p[1] eq 'FLIPX')
      {
        if(!$bitmaps8x8{$p[2]})
        {
          print STDERR "Can't find bitmap $p[2] for flipx source! $linenum \@ $filename\n";
          exit(1);
        }
        $b8='';
        $b8.=&reversebitorder($_) foreach(split('',$bitmaps8x8{$p[2]}));
        $b16='';
        $b16.=&reversebitorder($_) foreach(split('',$bitmaps8x16{$p[2]}));
        # no 16x16 support here yet
        $bitmaps8x8{$charname} = $b8;
        $bitmaps8x16{$charname} = $b16;
      }
      elsif($p[1] eq 'FLIPY')
      {
        if(!$bitmaps8x8{$p[2]})
        {
          print STDERR "Can't find bitmap $p[2] for flipy source! $linenum \@ $filename\n";
          exit(1);
        }
        $b8='';
        $b8=$_.$b8 foreach(split('',$bitmaps8x8{$p[2]}));
        $b16='';
        $b16=$_.$b16 foreach(split('',$bitmaps8x16{$p[2]}));
        # no 16x16 support here yet
        $bitmaps8x8{$charname} = $b8;
        $bitmaps8x16{$charname} = $b16;
      }
      elsif($p[1] eq 'INV')
      {
        if(!$bitmaps8x8{$p[2]})
        {
          print STDERR "Can't find bitmap $p[2] for inv source! $linenum \@ $filename\n";
          exit(1);
        }
        $b8='';
        $b8.=chr(255-ord($_)) foreach(split('',$bitmaps8x8{$p[2]}));
        $b16='';
        $b16.=chr(255-ord($_)) foreach(split('',$bitmaps8x16{$p[2]}));
        $bitmaps8x8{$charname} = $b8;
        $bitmaps8x16{$charname} = $b16;
      }
    }
    elsif($p[0] eq '8x8' || $p[0] eq '8x16' || $p[0] eq '16x16')
    {
      if($bitline>=1 && $bitline!=$height)
      {
        print "Bitmap not $height pixels high! <$linenum \@ $filename\n";
        exit(1);
      }
      $mode=$p[0];
      ($width,$height)=split('x',$mode);
      $bitline=0;
      $bitmap='';
    }
    elsif($charname ne 'IGNORE')
    {
      if($p[0] =~ /^[\.\#]+$/)
      {
        if(length($p[0])!=$width)
        {
          print "Bitmap not $width pixels wide! $linenum \@ $filename\n";
          exit(1);
        }
        tr/\.\#/01/;
        
        $bitline++;
        if($bitline<=$height)
        {
        if($width==8)
        {
          $bitmap.=pack('B8',$_);
        } else
        {
          $bitmap.=pack('B16',$_);
        }
        
        if($bitline==$height)
        {
          if($height==8)
          {
            $bitmaps8x8{$charname} = $bitmap;
            if($bitmaps8x16{$charname} eq '')
            {
              $bitmaps8x16{$charname} = &doublebytes($bitmap);
            }
          }
          elsif($height==16)
          {
            if($width==8)
            {
              $bitmaps8x16{$charname} = $bitmap;
            } else
            {
              $bitmaps16x16{$charname} = $bitmap;
            }
          }
        }
        } else
        {
          printf "Exceeding given bitmap height $linenum \@ $filename\n";
          exit(1);
        }
      }
    }
  }
  close(F);
}

$patch=$ARGV[0] if($#ARGV>=0);
&readin('unscii.txt');
&dumpfont();
&dumpconvtab();

print "XTND $xtndpointer XTND2 $xtnd2pointer XTND3 $xtnd3pointer\n";
