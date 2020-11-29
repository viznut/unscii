#!/usr/bin/env perl

# checks width errors in a 8x16 hex font

use Text::CharWidth qw(mbwidth mbswidth mblen);

while(<>)
{
  chop;
  ($hex,$data)=split(':');
  $dataw=length($data)/4;
  $corrw=8*mbwidth(chr(hex($hex)));
  if($corrw>0 && $dataw!=$corrw)
  {
    print $hex," : width $dataw should be $corrw\n"
  }
}
