; Hello world in assembly with libc
; compile with
;       `nasm -f elf64 c.asm -o c.o && gcc c.o -o c`

        default rel

        global _start
        extern puts

        section .text
_start:
        lea rdi, [message]
        call puts wrt ..plt      ; puts(&message)
        xor eax, eax
        ret

        section .rodata
message:
        db "Hello world!", 0    ; null terminated string