all:
	make -f ./MakefileLang
	make -C ./Compiler Makefile
