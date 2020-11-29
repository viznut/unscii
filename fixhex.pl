#!/usr/bin/env perl

$prevcp='';
while(<>) {
  ($cp,$bm)=split(':',$_);
  if(length($cp)!=5) { $cp=substr('00000'.$cp,-5); }
  print $cp.':'.$bm if($cp ne $prevcp);
  $prevcp=$cp;
}
