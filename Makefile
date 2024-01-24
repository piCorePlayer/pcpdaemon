
OBJS_SERVER = pcpd.o
OBJS_CLIENT = pcpd_cli.o
LIBS_SERVER = 
LIBS_CLIENT = 

CFLAGS = -c
CC = gcc

PROS = pcpd_cli pcpd

all: $(PROS)

.c.o:
	$(CC) $(CFLAGS) $<

pcpd_cli: $(OBJS_CLIENT)
	$(CC) -o $@ $^ $(LIBS_CLIENT)

pcpd: $(OBJS_SERVER)
	$(CC) -o $@ $^ $(LIBS_SERVER)

clean:
	rm -rf $(PROS) $(OBJS_CLIENT) $(OBJS_SERVER)
