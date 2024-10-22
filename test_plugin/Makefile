
DEBUG=-DDEBUG
FLAGS=-Wall -O2 -fPIC $(DEBUG)

default:
	gcc -o test_plugin.so test_plugin.c -shared $(FLAGS)

clean:
	@rm -rf test_plugin.so
	@echo "Clean!"
