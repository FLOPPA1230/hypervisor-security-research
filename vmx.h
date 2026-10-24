#ifndef VMX_H
#define VMX_H

#include <stdint.h>
#include <stddef.h>

#define VMX_ERR_SUCCESS 0
#define VMX_ERR_FAILURE 1

#define IA32_VMX_BASIC_MSR 0x480
#define IA32_VMX_PINBASED_CTLS_MSR 0x481
#define IA32_VMX_PROCBASED_CTLS_MSR 0x482
#define IA32_VMX_EXIT_CTLS_MSR 0x483
#define IA32_VMX_ENTRY_CTLS_MSR 0x484
#define IA32_VMX_MISC_MSR 0x485
#define IA32_VMX_CR0_FIXED0_MSR 0x486
#define IA32_VMX_CR0_FIXED1_MSR 0x487
#define IA32_VMX_CR4_FIXED0_MSR 0x488
#define IA32_VMX_CR4_FIXED1_MSR 0x489
#define IA32_VMX_VMCS_ENUM_MSR 0x48A
#define IA32_VMX_PROCBASED_CTLS2_MSR 0x48B
#define IA32_VMX_EPT_VPID_CAP_MSR 0x48C

#define VMX_EPT_DEFAULT_MEMORY_TYPE 6

#define VMX_EXIT_REASON_EXCEPTION 0
#define VMX_EXIT_REASON_CPUID 10
#define VMX_EXIT_REASON_RDMSR 31
#define VMX_EXIT_REASON_WRMSR 32
#define VMX_EXIT_REASON_EPT_VIOLATION 48
#define VMX_EXIT_REASON_EPT_MISCONFIG 49
#define VMX_EXIT_REASON_VMX_PREEMPTION 52

#define VMCS_FIELD_GUEST_ES_SEL 0x0800
#define VMCS_FIELD_GUEST_CS_SEL 0x0802
#define VMCS_FIELD_GUEST_SS_SEL 0x0804
#define VMCS_FIELD_GUEST_DS_SEL 0x0806
#define VMCS_FIELD_GUEST_FS_SEL 0x0808
#define VMCS_FIELD_GUEST_GS_SEL 0x080A
#define VMCS_FIELD_GUEST_LDTR_SEL 0x080C
#define VMCS_FIELD_GUEST_TR_SEL 0x080E
#define VMCS_FIELD_GUEST_RIP 0x681E
#define VMCS_FIELD_GUEST_RSP 0x681C
#define VMCS_FIELD_GUEST_RFLAGS 0x6820
#define VMCS_FIELD_GUEST_CR0 0x6800
#define VMCS_FIELD_GUEST_CR3 0x6802
#define VMCS_FIELD_GUEST_CR4 0x6804
#define VMCS_FIELD_HOST_CR0 0x6C00
#define VMCS_FIELD_HOST_CR3 0x6C02
#define VMCS_FIELD_HOST_CR4 0x6C04
#define VMCS_FIELD_HOST_RIP 0x6C16
#define VMCS_FIELD_HOST_RSP 0x6C14
#define VMCS_FIELD_HOST_ES_SEL 0x0C00
#define VMCS_FIELD_HOST_CS_SEL 0x0C02
#define VMCS_FIELD_HOST_SS_SEL 0x0C04
#define VMCS_FIELD_HOST_DS_SEL 0x0C06
#define VMCS_FIELD_HOST_FS_SEL 0x0C08
#define VMCS_FIELD_HOST_GS_SEL 0x0C0A
#define VMCS_FIELD_HOST_TR_SEL 0x0C0C

#define VMCS_FIELD_CTRL_PIN_BASED 0x4000
#define VMCS_FIELD_CTRL_CPU_BASED 0x4002
#define VMCS_FIELD_CTRL_EXCEPTION_BITMAP 0x4004
#define VMCS_FIELD_CTRL_EXIT 0x400C
#define VMCS_FIELD_CTRL_ENTRY 0x4012
#define VMCS_FIELD_CTRL_MSR_BITMAPS 0x4006
#define VMCS_FIELD_CTRL_EPTP 0x201A

#define VMCS_FIELD_VMEXIT_INSTRUCTION_LENGTH 0x440C

typedef struct __attribute__((packed)) {
    uint32_t revision_id;
    uint32_t abort_indicator;
    uint8_t  data[4088];
} vmxon_region_t;

typedef struct __attribute__((packed)) {
    uint32_t revision_id;
    uint32_t abort_indicator;
    uint8_t  data[4088];
} vmcs_region_t;

typedef struct __attribute__((packed)) {
    uint64_t guest_cr0;
    uint64_t guest_cr3;
    uint64_t guest_cr4;
    uint64_t guest_rip;
    uint64_t guest_rsp;
    uint64_t guest_rflags;
} guest_state_t;

uint8_t vmx_vmread8(uint64_t field);
uint16_t vmx_vmread16(uint64_t field);
uint32_t vmx_vmread32(uint64_t field);
uint64_t vmx_vmread64(uint64_t field);

void vmx_vmwrite8(uint64_t field, uint8_t value);
void vmx_vmwrite16(uint64_t field, uint16_t value);
void vmx_vmwrite32(uint64_t field, uint32_t value);
void vmx_vmwrite64(uint64_t field, uint64_t value);

int vmx_enable(void);
void vmx_disable(void);
int vmx_vmclear(void *vmcs_phys);
int vmx_vmptrld(void *vmcs_phys);

#endif