all:
	arm-linux-gnueabihf-gcc common.c update.c -o update11
clean:
	rm update11
