all:
	make -f ./MakefileLang
	make -C ./SPU-Processor Makefile

clean:
	rm -f bin/*.o bin/*.d
