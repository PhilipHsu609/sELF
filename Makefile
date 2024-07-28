ASM=c fib triangle

all: main asm

asm: $(ASM)

c: asm/c.asm
	nasm -f elf64 asm/c.asm -o c.o && gcc -no-pie c.o -o c

fib: asm/fib.asm
	nasm -f elf64 asm/fib.asm -o fib.o && gcc -no-pie fib.o -o fib

triangle: asm/triangle.asm
	nasm -f elf64 asm/triangle.asm -o triangle.o && ld triangle.o -o triangle	

main: main.o
	gcc main.o -o main

main.o: main.c
	gcc -c -g main.c

clean:
	rm -f main c fib triangle *.o
