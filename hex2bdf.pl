#!/usr/bin/perl
#
# Copyright (C) 1998, 2013 Roman Czyborra, Paul Hardy
# (Adapted for Unscii project by viznut in 2014)
#
# LICENSE:
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
#  
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
#    GNU General Public License for more details.
#  
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#    First, parse command-line options.  Two are available:
#
#       - Font name (-f name or --font name)
#       - Lines per glyph (-lnn or --lines nn, where nn is a decimal number)
#
use Getopt::Long;

$result = GetOptions (
             "copyright|c=s" => \$copyright, # Copyright string
             "font|f=s"      => \$font_name, # XLFD FAMILY_NAME
             "rows|r=i"      => \$vpixels,   # XLFD PIXEL_SIZE; vertical pixels
             "version|v=s"   => \$version,   # Version of this font
             "variant|V=s"   => \$variant
             );

if (not $font_name) {
   $font_name = "Unscii";
}
if (not $vpixels) {
   $vpixels  = 16;
}
if (not $version) {
   $version = "1.2";
}
if (not $foundry) {
   $foundry = "Unscii";
}
if (not $variant) {
   $variant = "Medium";
}

if($vpixels==16)
{
  ($caphgt,$xhgt,$asc,$desc,$undpos)=(11,7,13,3,1);
} else
{
  ($caphgt,$xhgt,$asc,$desc,$undpos)=(7,5,7,1,0);
}

$point_size   = $vpixels;
$point_size10 = 10 * $point_size;

$foundrylowcase = $foundry;
$founrdylowcase =~ tr/A-Z/a-z/;

## bdftopcf #defines MAXENCODING as 0xffff
#while (<>) { chomp; $glyph{$1} = $2 if /^([0-9a-fA-F]+):([0-9a-fA-F]+)/; }
while (<>) { chomp; $glyph{$1} = $2 if /^(0....):(.+)/; }
@chars = sort keys %glyph;
$nchars = $#chars + 1;

print "STARTFONT 2.1
FONT -${foundrylowcase}-${font_name}-Medium-R-Normal-${variant}-${vpixels}-${point_size10}-75-75-c-80-iso10646-1
SIZE $point_size 75 75
FONTBOUNDINGBOX $vpixels $vpixels 0 -2
STARTPROPERTIES 24
COPYRIGHT \"${copyright}\"
FONT_VERSION \"${version}\"
FONT_TYPE \"Bitmap\"
FOUNDRY \"${foundry}\"
FAMILY_NAME \"${font_name}\"
WEIGHT_NAME \"Medium\"
SLANT \"R\"
SETWIDTH_NAME \"Normal\"
ADD_STYLE_NAME \"${variant}\"
PIXEL_SIZE ${vpixels}
POINT_SIZE ${point_size10}
RESOLUTION_X 75
RESOLUTION_Y 75
SPACING \"C\"
AVERAGE_WIDTH 80
CHARSET_REGISTRY \"ISO10646\"
CHARSET_ENCODING \"1\"
UNDERLINE_POSITION ${undpos}
UNDERLINE_THICKNESS 1
CAP_HEIGHT ${caphgt}
X_HEIGHT ${xhgt}
FONT_ASCENT ${vpixels}
FONT_DESCENT 0
DEFAULT_CHAR 63
ENDPROPERTIES
CHARS $nchars\n";

foreach $character (@chars)
{
	$encoding = hex($character); $glyph = $glyph{$character};
	$width    = length ($glyph) / $vpixels;  # hex digits per row
	$dwidth   = $width * 4;     # device width, in pixels; 1 digit = 4 px
	# scalable width, units of 1/1000th full-width glyph
	$swidth   = (1000 * $width) / ($vpixels / 4);
	$glyph    =~ s/((.){$width})/\n$1/g;
	$character = "$character $charname"
	    if $charname = $charname{pack("n",hex($character))};

	print "STARTCHAR U+$character
ENCODING $encoding
SWIDTH $swidth 0
DWIDTH $dwidth 0
BBX $dwidth $vpixels 0 0
BITMAP $glyph
ENDCHAR\n";
}

print "ENDFONT\n";
