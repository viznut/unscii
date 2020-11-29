#!/usr/bin/env perl

while(<>)
{
  chop;
  @a=split(':',$_);
  print $a[0].':';
  for($i=0;$i<8;$i++) { print substr($a[1],$i*2,2) x 2; }
  print "\n";
}
