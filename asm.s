.intel_syntax noprefix
.text

.global __vmx_vmread
.global __vmx_vmwrite
.global __vmx_on
.global __vmx_off
.global __vmx_vmclear
.global __vmx_vmptrld
.global __vmlaunch
.global __vmresume
.global __invept

__vmx_vmread:
    push rbp
    mov rbp, rsp
    mov rcx, rdi
    vmread rax, rcx
    leave
    ret

__vmx_vmwrite:
    push rbp
    mov rbp, rsp
    mov rcx, rdi
    mov rdx, rsi
    vmwrite rcx, rdx
    leave
    ret

__vmx_on:
    push rbp
    mov rbp, rsp
    mov rax, rdi
    vmxon [rax]
    setna al
    movzx rax, al
    leave
    ret

__vmx_off:
    push rbp
    mov rbp, rsp
    vmxoff
    leave
    ret

__vmx_vmclear:
    push rbp
    mov rbp, rsp
    mov rax, rdi
    vmclear [rax]
    setna al
    movzx rax, al
    leave
    ret

__vmx_vmptrld:
    push rbp
    mov rbp, rsp
    mov rax, rdi
    vmptrld [rax]
    setna al
    movzx rax, al
    leave
    ret

__vmlaunch:
    push rbp
    mov rbp, rsp
    vmlaunch
    setna al
    movzx rax, al
    leave
    ret

__vmresume:
    push rbp
    mov rbp, rsp
    vmresume
    setna al
    movzx rax, al
    leave
    ret

__invept:
    push rbp
    mov rbp, rsp
    mov rcx, rdi
    mov rdx, rsi
    invept rcx, [rdx]
    leave
    ret