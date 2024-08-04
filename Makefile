test=hello-c hello-asm

all: main test

test: $(test)

hello-c: test/hello.c
	gcc -o hello-c test/hello.c

hello-asm: test/hello.asm
	nasm -f elf64 test/hello.asm -o hello-asm.o
	ld --dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie hello-asm.o -o hello-asm

main: main.o
	gcc main.o -o main

main.o: main.c
	gcc -c -g main.c

clean:
	rm -f main hello-c hello-asm *.o
