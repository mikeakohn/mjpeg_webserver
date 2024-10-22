include ../config.mak

VPATH=../src

all: $(OBJS)
	$(COMPILER_PREFIX)$(CC) -o ../mjpeg_webserver$(CONFIG_EXT) \
	  ../src/mjpeg_webserver.c \
	  $(OBJS) $(FLAGS) \
	  $(CFLAGS) \
	  $(LDFLAGS)
	@if [ -f mjpeg_webserver.exe ]; then mv mjpeg_webserver.exe ..; fi
	@if [ -f mjpeg_webserver ]; then mv mjpeg_webserver ..; fi

%.o: %.c *.h
	$(COMPILER_PREFIX)$(CC) -c $< -o $*.o $(FLAGS) $(CFLAGS)

%.o: %.c
	$(COMPILER_PREFIX)$(CC) -c $< -o $*.o $(FLAGS) $(CFLAGS)

#$(MODULES1):
#	$(COMPILER_PREFIX)$(CC) -c $*.c $(FLAGS) $(CFLAGS)

install:
	cp mjpeg_webserver /usr/local/bin
	cp mjpeg_webserver.conf /usr/local/bin/etc

clean:
	@rm -f *.o mjpeg_webserver mjpeg_webserver.exe

