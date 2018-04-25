all:
	arm-linux-gnueabihf-gcc common.c update.c -o update
clean:
	rm update
