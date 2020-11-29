SRC=unscii.txt punctuation.txt numbers.txt math.txt textsymbols.txt \
latin.txt greek.txt cyrillic.txt hebrew.txt arabic.txt katakana.txt runes.txt \
diacritics.txt symbols.txt arrows.txt shapes.txt lines.txt patterns.txt \
divisions.txt grids.txt pictures.txt ctrl.txt

CC=gcc -Os

all: unscii-16.pcf unscii-8.pcf unscii-16-full.pcf \
     unscii-8-alt.pcf unscii-8-thin.pcf unscii-8-tall.pcf unscii-8-mcr.pcf unscii-8-fantasy.pcf \
     unscii-16.ttf unscii-8.ttf unscii-16-full.ttf \
     unscii-8-alt.ttf unscii-8-thin.ttf unscii-8-tall.ttf unscii-8-mcr.ttf unscii-8-fantasy.ttf \
     bm2uns

VERSION=2.1

HEX2BDF=./hex2bdf.pl --version=$(VERSION)

### HEX ###

unscii-16.hex: $(SRC)
	./assemble.pl

unscii-8.hex: $(SRC)
	./assemble.pl

unscii-8-alt.hex: $(SRC) font-alt.txt
	./assemble.pl alt

unscii-8-thin.hex: $(SRC) font-thin.txt
	./assemble.pl thin

unscii-8-mcr.hex: $(SRC) font-mcr.txt
	./assemble.pl mcr

unscii-8-fantasy.hex: $(SRC) font-fantasy.txt
	./assemble.pl fantasy

unscii-8-tall.hex: unscii-8.hex
	./doubleheight.pl < unscii-8.hex > unscii-8-tall.hex

unscii-16-full.hex: unscii-16.hex unifont.hex fsex-adapted.hex
	./merge-otherfonts.pl > unscii-16-full.hex

### PCF ###

unscii-16.pcf: unscii-16.hex
	$(HEX2BDF) --variant='16' --rows=16 < unscii-16.hex | bdftopcf > unscii-16.pcf

unscii-8.pcf: unscii-8.hex
	$(HEX2BDF) --variant='8' --rows=8 < unscii-8.hex | bdftopcf > unscii-8.pcf

unscii-8-alt.pcf: unscii-8-alt.hex
	$(HEX2BDF) --variant='alt' --rows=8 < unscii-8-alt.hex | bdftopcf > unscii-8-alt.pcf

unscii-8-thin.pcf: unscii-8-thin.hex
	$(HEX2BDF) --variant='thin' --rows=8 < unscii-8-thin.hex | bdftopcf > unscii-8-thin.pcf

unscii-8-tall.pcf: unscii-8-tall.hex
	$(HEX2BDF) --variant='tall' --rows=16 < unscii-8-tall.hex | bdftopcf > unscii-8-tall.pcf

unscii-8-mcr.pcf: unscii-8-mcr.hex
	$(HEX2BDF) --variant='mcr' --rows=8 < unscii-8-mcr.hex | bdftopcf > unscii-8-mcr.pcf

unscii-8-fantasy.pcf: unscii-8-fantasy.hex
	$(HEX2BDF) --variant='fantasy' --rows=8 < unscii-8-fantasy.hex | bdftopcf > unscii-8-fantasy.pcf

unscii-16-full.pcf: unscii-16-full.hex
	$(HEX2BDF) --variant='full' --rows=16 < unscii-16-full.hex | bdftopcf > unscii-16-full.pcf

### SVG ###

unscii-16.svg: unscii-16.hex vectorize
	./vectorize 16 16 < unscii-16.hex > unscii-16.svg

unscii-8.svg: unscii-8.hex vectorize
	./vectorize 8 8 < unscii-8.hex > unscii-8.svg

unscii-8-alt.svg: unscii-8-alt.hex vectorize
	./vectorize 8 alt alt < unscii-8-alt.hex > unscii-8-alt.svg

unscii-8-thin.svg: unscii-8-thin.hex vectorize
	./vectorize 8 thin < unscii-8-thin.hex > unscii-8-thin.svg

unscii-8-tall.svg: unscii-8-tall.hex vectorize
	./vectorize 8 tall < unscii-8-tall.hex > unscii-8-tall.svg

unscii-8-mcr.svg: unscii-8-mcr.hex vectorize
	./vectorize 8 mcr < unscii-8-mcr.hex > unscii-8-mcr.svg

unscii-8-fantasy.svg: unscii-8-fantasy.hex vectorize
	./vectorize 8 fantasy < unscii-8-fantasy.hex > unscii-8-fantasy.svg

unscii-16-full.svg: unscii-16-full.hex vectorize
	./vectorize 16 full < unscii-16-full.hex > unscii-16-full.svg

### TTF/OTF/WOFF ###

unscii-16.ttf: unscii-16.svg
	./makevecfonts.ff unscii-16

unscii-8.ttf: unscii-8.svg
	./makevecfonts.ff unscii-8

unscii-8-alt.ttf: unscii-8-alt.svg
	./makevecfonts.ff unscii-8-alt

unscii-8-thin.ttf: unscii-8-thin.svg
	./makevecfonts.ff unscii-8-thin

unscii-8-tall.ttf: unscii-8-tall.svg
	./makevecfonts.ff unscii-8-tall

unscii-8-mcr.ttf: unscii-8-mcr.svg
	./makevecfonts.ff unscii-8-mcr

unscii-8-fantasy.ttf: unscii-8-fantasy.svg
	./makevecfonts.ff unscii-8-fantasy

unscii-16-full.ttf: unscii-16-full.svg
	./makevecfonts.ff unscii-16-full

### tools ###

uns2uni.tr: $(SRC)
	./assemble.pl

vectorize: vectorize.c
	$(CC) vectorize.c -o vectorize

bm2uns: bm2uns.c bm2uns.i
	$(CC) -O3 bm2uns.c -o bm2uns `sdl-config --libs --cflags` -lSDL_image -lm

bm2uns.i: unscii-8.hex bm2uns-prebuild.pl
	./bm2uns-prebuild.pl | sort > bm2uns.i

uns2uni: uns2uni.tr makeconverters.pl
	./makeconverters.pl

uni2uns: uns2uni.tr makeconverters.pl
	./makeconverters.pl

### release ###

clean:
	rm -f *~ *.pcf *.svg unscii*.hex vectorize bm2uns bm2uns.i *.ttf *.otf *.woff \
	uns2uni.tr uns2uni uni2uns DEADJOE

srcpackage: clean
	cd .. && tar czf unscii-$(VERSION)-src.tar.gz unscii-$(VERSION)-src

web:
	cp *.pcf *.ttf *.otf *.woff *.hex ../web/
	cp ../unscii-$(VERSION)-src.tar.gz ../web/
