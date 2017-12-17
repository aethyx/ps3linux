/*
 * lboot.ld
 */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
	. = 0x100000;

	.head : { *(.head) }

	. = ALIGN(4096);
	.kernel : { *(.kernel) }

 	. = ALIGN(4096);
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(.bss) }

 	. = ALIGN(4096);
 	.initrd : { *(.initrd) }
}

