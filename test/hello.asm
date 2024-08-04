; Hello world assembly
; compile with
;       `nasm -f elf64 hello.asm -o hello-asm.o && ld --dynamic-linker /lib64/ld-linux-x86-64.so.2 -pie hello-asm.o -o hello-asm`

        default rel

        global _start

        section .text
_start:
        mov rdi, 1
        lea rsi, [message]
        mov rdx, 14

        mov rax, 1
        syscall
        
        mov rdi, 0

        mov rax, 60
        syscall

        section .rodata
message:
        db "Hello world!", 10, 0    ; null terminated string