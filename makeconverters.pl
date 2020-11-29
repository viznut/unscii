#!/usr/bin/env perl

open(F,'uns2uni.tr');
$uns=<F>;
$uni=<F>;
chop $uns;

open(CT,">uns2uni");
print CT "#!/usr/bin/env ruby\n".
         "print STDIN.read.tr('$uns','$uni')";
close(CT);
open(CT,">uni2uns");
print CT "#!/usr/bin/env ruby\n".
         "print STDIN.read.tr('$uni','$uns')";
close(CT);
