all:
	make -f ./MakefileLang
	make -C ./SPU-Processor Makefile

clean:
	rm -rf bin/backend/*.o bin/backend/*.d bin/frontend/*.o bin/frontend/*.d bin/src/*.o bin/src/*.d bin/hash/*.o bin/hash/*.d
