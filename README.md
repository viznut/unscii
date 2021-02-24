# UNSCII

A bitmapped Unscii font based on classic system fonts.

See http://viznut.fi/unscii/ for more.

Font compilation starts from the file src/unscii.txt which also contains
some more documentation, including information about the rest of the files
in the src directory.

The .hex format is basically the same as in the Unifont project. Each line
consists of codepoint:hexbitmap, and the length of the bitmap string
indicates whether the glyph is 8x8, 8x16 or 16x16.

The program code in this directory:
- Makefile: builds the font files and some other stuff
- assemble.pl: compiles the files under src/ into .hex files.
- bm2uns.c: converts arbitrary bitmaps into unscii art (needs SDL_image)
- bm2uns-prebuild.pl: builds some tables for bm2uns.c
- checkwidths.pl: checks if the glyph widths in a .hex file match terminal usage
- doubleheight.pl: stretches the 8x8 glyphs in a .hex file into 8x16
- hex2bdf.pl: converts a .hex file into X11 .bdf format (from Unifont)
- makeconverters.pl: makes shell scripts that convert between Unicode and legacy Unscii
- makevecfonts.ff: a Fontforge script to build ttf/otf/woff fonts from an .svg font
- merge-otherfonts.pl: fills in the missing glyphs from unifont.hex and fsex-adapte.hex
- vectorize.c: builds a .svg font file from a .hex file

Other files in this directory:
- fsex-adapted.hex: Fixedsys Excelsior, an older public domain font with some similarities to Unscii
- unifont.hex: Unifont, the definitive Unicode bitmap font. see https://savannah.gnu.org/projects/unifont/
