%define sys_exit 60
%define sys_read 0
%define sys_write 1

%define num_buf 0x402500
%define buf     0x402600

%define stdin 0
%define stdout 1

%define success 0

section .text

jmp _IOLIB_INPUT
jmp _IOLIB_OUTPUT

bits 64


;-----------------------------------|
; Input:    number in rsi           |
; Ouput:    NONE                    |
; Destr:                            |
;-----------------------------------|

_IOLIB_INPUT:
        mov rax, sys_read
        mov rdi, stdin
        mov rsi, num_buf
        inc rsi
        mov rdx, 256
        syscall

        mov rsi, num_buf
        inc rsi
        call _IOLIB_STR2DEC
        ret

;----------------------------------|
; Function for printing a number   |
;----------------------------------|
; Input: number in rax             |
; Output:                          |
; Destr: rcx, rsi, rdx             |
;----------------------------------|

_IOLIB_OUTPUT:
        push rcx
        push rsi
        push rdx
        push rbx
        push rdi

        mov rsi, num_buf
        mov rdi, buf

        call _IOLIB_DEC2STR

        mov rsi, buf
        mov rdi, 1
        call _IOLIB_LINELEN
        mov rdx, rax
        mov rsi, buf
        mov rax, sys_write
        syscall

        mov rdi, num_buf
        mov rcx, 128
        xor rax, rax
        rep stosb

        mov rdi, buf
        mov rcx, 128
        rep stosb

        pop rdi
        pop rbx
        pop rdx
        pop rsi
        pop rcx
        ret

;-----------------------------------|
; Function for cleaning buffer      |
; Inserting 0 bytes                 |
;-----------------------------------|
; Entry         Buf address in rsi  |
; Return        Void                |
; Destr                             |
;-----------------------------------|

CleanBuf:



;-----------------------------------|
; Function for counting line length |
; ----------------------------------|
; Entry:        Line address in rsi |
; Return:       Line length in  rax |
; Destr:        rcx, r12            |
; ----------------------------------|

_IOLIB_LINELEN:
        xor rbx, rbx
        chzr:
                lodsb
                test al, al
                jz chzr_end
                inc rbx
                jmp chzr

        chzr_end:
        mov rax, rbx
        ret


;-------------------------------------------|
; Function for turning dec number to string |
;-------------------------------------------|
; Entry: number in rax, buf in rsi          |
; Exit:  dec number in buf                  |
; Destr: rax, rbx, rdx                      |
;-------------------------------------------|

_IOLIB_DEC2STR:
        push r11
        mov r11, num_buf
        inc r11

        push rdx
        push rbx
        xor rbx, rbx

        push rax

        push rcx

        mov rcx, rax
        and ecx, 0x80000000
        test ecx, ecx
        jne neg_ecx
        pop rcx

        neg_ecx_cont:
        mov rbx, 10

        decnum2str_loop:

        xor rdx, rdx

        div rbx

        add dl, 0x30            ; adding 30 to mod

        mov [r11], dl           ; copying to buf
        inc r11                 ; moving buf pointer

        test rax, rax           ; checking if zero
        jz decnum2str_end

        jmp decnum2str_loop


        decnum2str_end:
        dec r11

        pop rax

        push rsi
        mov rsi, r11

        deccopy_num2str:
                std
                lodsb

                inc rsi
                mov byte [rsi], 0
                dec rsi

                cld
                stosb

                cmp rsi, num_buf
                je deccopy_num2str_end
                jmp deccopy_num2str

        deccopy_num2str_end:

        mov rax, 0xa
        stosb

        pop rsi
        pop rbx
        pop rdx
        pop r11
        ret

neg_ecx:
        pop rcx
        mov byte [rdi], '-'
        inc rdi

        neg eax

        pop rbx

        push rax

        jmp neg_ecx_cont


;---------------------------------------
; Function for turning string to decimal
;---------------------------------------
; Entry:  buffer address in rsi
; Output: int number in rax
; Destr:
;---------------------------------------

_IOLIB_STR2DEC:
        push rbx
        push rcx
        push rsi
        call _IOLIB_LINELEN
        pop rsi

        xor     rax, rax
        xor     rcx, rcx
        mov     rbx, 10
.nextchar:
        lodsb
        cmp     al, '0'
        jb      .done
        cmp     al, '9'
        ja      .done

        imul    rcx, rbx
        sub     al, '0'
        add     rcx, rax
        jmp     .nextchar
.done:
        mov rax, rcx
        pop rcx
        pop rbx
        ret


; section .data


;
; num_buf times   256 db 0
; buf     times   256 db 0
; alphabet db     '0123456789ABCDEF'
; number db 0, "156", 0
