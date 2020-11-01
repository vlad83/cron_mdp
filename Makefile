all: scheduler.Makefile
	make -f scheduler.Makefile

clean:
	rm *.o -f
	rm *.elf -f
