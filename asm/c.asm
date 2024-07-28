; C in assembly
; compile with
;       `nasm -f elf64 c.asm && gcc -no-pie c.o

        global main
        extern puts

        section .text
main:
        mov rdi, message
        call puts               ; puts(&message)
        ret

message:
        db "Hello world!", 0    ; null terminated string