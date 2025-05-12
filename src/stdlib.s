section     .text

BuffSize equ 100h

;================================================================================
;OUT: RSI - number
;================================================================================
getNum:
    xor rax, rax            ; read64 (rdi, rsi, rdx)
    mov rdi, 0              ; stdin
    mov rsi, readBuffer     ; buffer
    mov rdx, BuffSize
    syscall

    mov rsi, readBuffer
    xor rdx, rdx
    loopConvert:
    lodsb

    cmp al, '0'
    jl stopConvert

    cmp al, '9'
    jg stopConvert

    sub al, '0'

    shl rdx, 1d
    mov rbx, rdx
    shl rdx, 2d
    add rdx, rbx

    add rdx, rax
    jmp loopConvert

    stopConvert:
    mov rsi, rdx

    ret
;================================================================================



;================================================================================
;IN: RSI - number to print
;================================================================================
printNum:
    mov r11, Buffer
    xor rdx, rdx

    call printNumDec
    call printBuff
    ret
;================================================================================



;================================================================================
exitProg:
    mov rax, 0x3c           ; func to exit
    xor rdi, rdi            ; exit code
    syscall
;================================================================================



;================================================================================
;Comm: prints num to buffer, in decimal.
;In: R11 + RDX - place in buffer,  RSI - number
;Destr: RAX, RBX, RDX, RDI, R12, R10
;================================================================================
printNumDec:
    push rbx

    mov r12, rsi
    and r12, 0xf0000000
    cmp r12, 0xf0000000
    jne skipMinus
    xor rsi, -1d
    inc rsi

    mov r10, '-'
    call putChar

    skipMinus:

    xor r12, r12
    xor rcx, rcx
    mov rbx, rdx
    xor rdx, rdx
    xor rax, rax
    mov rdi, 10d

    loopNumDec:
        xor rdx, rdx
        mov rax, rsi
        div rdi
        mov rsi, rax

        mov r10, rdx
        add r10, alphabet

        xor rax, rax
        mov al, [r10]
        mov r10b, al

        mov [digitBuff + rcx], r10b
        inc rcx

        cmp rsi, 0h
        jne loopNumDec


    mov rdx, rbx
    loopGetDigNumDec:
        mov r10b, [digitBuff + rcx - 1]
        call putChar


        loop loopGetDigNumDec

    pop rbx
    ret

;================================================================================


;================================================================================
;Comm: prints one char to buff, clears and prints the buffer if it is full.
;Comm: saves RCX, might destr other regs
;In: R11 - buff, RDX - current pos in buff, R10 - char to print.
;Destr: RAX, RDI, RSI, RDX
;================================================================================
putChar:
    cmp rdx, BuffSize
    je clearBuff

    mov [r11 + rdx], r10b
    inc rdx

    jmp endClearBuff

    clearBuff:
    push rax
    push rbx
    push rcx
    push rdi
    push rsi
    push r10
    push r11

    call printBuff

    pop r11
    pop r10
    pop rsi
    pop rdi
    pop rcx
    pop rbx
    pop rax

    xor rdx, rdx
    call putChar

    endClearBuff:

    ret

;================================================================================


;================================================================================
;Comm: prints buffer to STDOUT, symbol count printed is BuffSize.
;In: RDX - chars to print.
;Destr: RAX, RDI, RSI.
;================================================================================
printBuff:
    mov rax, 0x01           ; write64 (rdi, rsi, rdx)
    mov rdi, 1              ; stdout
    mov rsi, Buffer         ; buffer
    syscall


    mov rax, 0x01           ; write64 (rdi, rsi, rdx)
    mov rdi, 1              ; stdout
    mov rsi, Space         ; buffer
    mov rdx, 1
    syscall

    ret

;================================================================================

section     .data

readBuffer: times BuffSize db "$"
Buffer:     times BuffSize db "0"
digitBuff:  times 64d      db "0"
meow:       times 64d      db "0"
Space:      dq 0xa
alphabet:   db "0123456789abcdef"
