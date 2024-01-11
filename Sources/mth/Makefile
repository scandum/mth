CFLAGS  = -DLINUX
LIBS    = -lz
CC      = gcc -Wall -g

OUTFILE = mth

OBJECTS01 =   telopt.o msdp.o mth.o net.o mud.o

$(OUTFILE): $(OBJECTS01) 
	$(CC) $(CFLAGS) $(OBJECTS01) -o $(OUTFILE) $(LIBS)

clean:
	rm -f $(OBJECTS01) 
