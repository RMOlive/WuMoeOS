[org 0x1000]

dw 0x55aa

mov si, loading
call print

detect_memory:
    xor ebx, ebx
    mov ax, 0
    mov es, ax
    mov edi, ards_buffer
    mov edx, 0x534d4150
.next:
    mov eax, 0xe820
    mov ecx, 20
    int 0x15
    jc error
    add di, cx
    inc dword [ards_count]
    cmp ebx, 0
    jnz .next
    mov si, detecting
    call print
    jmp prepare_protected_mode

prepare_protected_mode:
    cli
    in al, 0x92
    or al, 0b10
    out 0x92, al
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword code_selector:protect_mode

print:
    mov ah, 0x0e
    .next:
        mov al, [si]
        cmp al, 0
        jz .done
        int 0x10
        inc si
        jmp .next
    .done:
        ret

loading:
    db "Loading WuMoe OS...", 10, 13, 0
detecting:
    db "Datacting Memory Success...", 10, 13, 0

error:
    mov si, .msg
    call print
    hlt
    jmp $
    .msg db "Loading Error!!!", 10, 13, 0

[bits 32]
protect_mode:
	mov ax, data_selector
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, 0x10000
    mov edi, 0x10000
    mov ecx, 10
    mov bl, 200
    call read_disk
    mov eax, 0x20220609
    mov ebx, ards_count
    jmp dword code_selector:0x10000
    ud2

read_disk:
    mov dx, 0x1f2
    mov al, bl
    out dx, al
    inc dx
    mov al, cl
    out dx, al
    inc dx
    shr ecx, 8
    mov al, cl
    out dx, al
    inc dx
    shr ecx, 8
    mov al, cl
    out dx, al
    inc dx
    shr ecx, 8
    and cl, 0b1111
    mov al, 0b1110_0000
    or al, cl
    out dx, al
    inc dx
    mov al, 0x20
    out dx, al
    xor ecx, ecx
    mov cl, bl
    .read:
        push cx
        call .waits
        call .reads
        pop cx
        loop .read
    ret
    .waits:
        mov dx, 0x1f7
        .check:
            in al, dx
            jmp $+2
            jmp $+2
            jmp $+2
            and al, 0b1000_1000
            cmp al, 0b0000_1000
            jnz .check
        ret
    .reads:
        mov dx, 0x1f0
        mov cx, 256
        .readw:
            in ax, dx
            jmp $+2
            jmp $+2
            jmp $+2
            mov [edi], ax
            add edi, 2
            loop .readw
        ret

code_selector equ (1 << 3)
data_selector equ (2 << 3)

memory_base equ 0
memory_limit equ ((1024 * 1024 * 1024 * 4) / (1024 * 4)) - 1

gdt_ptr:
	dw (gdt_end - gdt_base) - 1
	dd gdt_base
gdt_base:
	dd 0, 0
gdt_code:
	dw memory_limit & 0xffff
	dw memory_base & 0xffff
	db (memory_base >> 16) & 0xff
	db 0b_1_00_1_1_0_1_0
	db 0b1_1_0_0_0000 | (memory_limit >> 16) & 0xf
	db (memory_base >> 24) & 0xff
gdt_data:
    dw memory_limit & 0xffff
    dw memory_base & 0xffff
    db (memory_base >> 16) & 0xff
    db 0b_1_00_1_0_0_1_0
    db 0b1_1_0_0_0000 | (memory_limit >> 16) & 0xf
    db (memory_base >> 24) & 0xff
gdt_end:

ards_count:
    dd 0
ards_buffer:
