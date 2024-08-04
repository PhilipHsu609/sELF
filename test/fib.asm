; calculate fibonacci number recursively in assembly with libc
; compile with
;       `nasm -f elf64 fib.asm && gcc fib.o -o fib`

        default rel

        global main
        extern printf
        extern strtol

        section .text
main:
        cmp rdi, 2
        jl .usage
        
        mov rdi, qword [rsi + 8]
        xor esi, esi
        mov edx, 10
        call strtol

        mov edi, eax
        call fib

        lea rdi, [fmt]
        mov rsi, rax
        mov eax, 0
        call printf
        xor eax, eax
        ret

.usage:
        lea rdi, [usage]
        mov eax, 0
        call printf
        xor eax, eax
        ret

fib:
        mov rdx, rdi
        cmp rdx, 1
        jg recur

        mov rax, rdx
        jmp fin

recur:
        push rcx
        push rdx

        mov rdi, rdx
        sub rdi, 1
        call fib

        pop rdx
        pop rcx

        mov rcx, rax
        
        push rcx
        push rdx

        mov rdi, rdx
        sub rdi, 2
        call fib

        pop rdx
        pop rcx

        add rcx, rax
        mov rax, rcx
fin:
        ret

fmt:
        db "%ld", 10, 0
usage:
        db "./fib [num]", 10, 0