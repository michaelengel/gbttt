GBCC = sdcc
GBAS = sdasgb
GBLD = sdldgb
GBCFLAGS = -c -msm83
GBLDFLAGS = -i -b _IVT=0x0000 -b _HEADER=0x0100 -b _CODE=0x0150 \
            -b _DATA=0xc000
GBASFLAGS = -o

cart.gb : cart.ihx ihx_to_bin
	./ihx_to_bin < cart.ihx > cart.gb

header.rel : header.asm
	$(GBAS) $(GBASFLAGS) header

cart.ihx : header.rel cart.rel
	$(GBLD) $(GBLDFLAGS) cart.ihx header.rel cart.rel

cart.rel : cart.c tiles.inc
	$(GBCC) $(GBCFLAGS) cart.c

tiles.inc : tiles.til convtiles
	./convtiles tiles.til tiles.inc tiles

ihx_to_bin : ihx_to_bin.c
convtiles : convtiles.c

clean :
	$(RM) cart.ihx cart.rel cart.lst cart.map cart.asm cart.noi cart.sym \
	      header.rel cart.lk ihx_to_bin cart.gb tiles.inc convtiles \#* *~
