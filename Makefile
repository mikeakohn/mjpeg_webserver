include config.mak

all:
	@make -C build

install:
	@if [ ! -d $(PREFIX)/bin ]; then mkdir $(PREFIX)/bin; fi;
	@if [ ! -d $(PREFIX)/etc ]; then mkdir $(PREFIX)/etc; fi;
	cp mjpeg_webserver /usr/local/bin
	cp mjpeg_webserver.conf /usr/local/bin/etc

clean:
	@rm -f mjpeg_webserver mjpeg_webserver.exe build/*.o
	@echo "Clean!"

distclean: clean
	@rm config.mak

