CC       = gcc
LD       = ld
AS       = gcc

CFLAGS   = -Iinclude -ffreestanding -fno-stack-protector -fshort-wchar \
           -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -Wall -Wextra -O2 -c \
           -fno-strict-aliasing -fno-omit-frame-pointer

ASFLAGS  = -Iinclude -ffreestanding -fno-stack-protector -fshort-wchar \
           -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -c

LDFLAGS  = -nostdlib -znocombreloc -shared -Bsymbolic -T elf_x86_64_efi.lds \
           -L /usr/lib -L /usr/lib/gnuefi -lgnuefi -lefi

OBJS     = src/main.o src/vmx2.o src/cpu.o src/ept.o src/msr.o src/hooks.o src/asm.o

TARGET   = stealth.efi

all: $(TARGET)

%.o: %.c
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) $< -o $@

%.o: %.S
	@echo "  AS      $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(TARGET): $(OBJS)
	@echo "  LD      $@"
	@$(LD) $(LDFLAGS) $(OBJS) -o $@

clean:
	@rm -f src/*.o $(TARGET)