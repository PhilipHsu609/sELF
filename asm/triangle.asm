; print a triangle
; compile with 
;       `nasm -f elf64 triangle.asm && ld triangle.o`
;
; *
; **
; ***
; ****

    global _start
    extern puts

    section .text
_start:
    mov rdx, output         ; ptr
    mov r8, 1               ; i
    mov r9, 0               ; j
line:
    mov byte [rdx], '*'     ; *ptr = '*'
    inc rdx                 ; ptr++
    inc r9                  ; j++
    cmp r9, r8              ; if j != i goto line
    jne line
done:
    mov byte [rdx], 10      ; *ptr = '\n'
    inc rdx                 ; ptr++
    inc r8                  ; i++
    mov r9, 0               ; j = 0
    cmp r8, max_line        ; if i <= max_line goto line
    jng line
finish:
    mov rax, 1
    mov rdi, 1
    mov rsi, output
    mov rdx, data_size
    syscall                 ; write(STDOUT_FILENO, output, 44)
    mov rax, 60
    xor rdi, rdi
    syscall                 ; exit(0)

    section .bss
max_line    equ 8
data_size   equ 44
output:     resb data_size