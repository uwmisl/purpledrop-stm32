/*
 * Copyright (c) 2011-2012, 2019, Fabian Greif
 * Copyright (c) 2012, 2015-2021, Niklas Hauser
 * Copyright (c) 2013, Sascha Schade
 * Copyright (c) 2013, 2015, Kevin Läufer
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * ------------------------------------------------------------------------- */


MEMORY
{
	FLASH_BOOT (rx) : ORIGIN = 0x00400000, LENGTH = 8K
	FLASH_CFG1 (r) : ORIGIN = 0x00402000, LENGTH = 4K
	FLASH_CFG2 (r) : ORIGIN = 0x00403000, LENGTH = 4k
	FLASH (rx) : ORIGIN = 0x00404000, LENGTH = 496K
	RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 163840
	CONT_RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 163840
}

/*__flash_start = ORIGIN(FLASH);
__flash_end = ORIGIN(FLASH) + LENGTH(FLASH);
__ram_start = ORIGIN(RAM);
__ram_end = ORIGIN(RAM) + LENGTH(RAM);
*/
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

/* the handler stack is used for main program as well as interrupts */
MAIN_STACK_SIZE    = 3072;
PROCESS_STACK_SIZE = 0;
TOTAL_STACK_SIZE   = MAIN_STACK_SIZE + PROCESS_STACK_SIZE;

__vec_offset = TOTAL_STACK_SIZE;
SECTIONS
{
	__rom_start = ORIGIN(FLASH);


	.vector_rom :
	{
		__vector_table_rom_start = .;
		__vector_table_ram_load = .;
		/* Initial stack address, Reset and NMI handler */
		KEEP(*(.vector_rom))
	} >FLASH


	.text :
	{
		*(.text .text.* .gnu.linkonce.t.*)
	} >FLASH

	.rodata :
	{
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		KEEP(*(.init))

		. = ALIGN(4);
		__init_array_start = .;
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		. = ALIGN(4);
		__init_array_end = .;

		__hardware_init_start = .;
		/* Table symbols are alphabetically sorted! */
		KEEP(*(SORT(.hardware_init.order_*)))
		KEEP(*(SORT(.hardware_init)))
		. = ALIGN(4);
		__hardware_init_end = .;

		__assertion_table_start = .;
		KEEP(*(.assertion))
		. = ALIGN(4);
		__assertion_table_end = .;

		__build_id = .;
		KEEP(*(.note.gnu.build-id))
	} >FLASH
	/* We do not call static destructors ever */
	/DISCARD/ :
	{
		*(.fini_array .fini_array.*)
	}

	/* Unwind tables are used to unwind the stack for C++ exceptions. Even
	 * though we disabled exceptions, pre-compiled libraries such as libstdc++
	 * still have to raise exceptions. */
	/DISCARD/ :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		*(.eh_frame*)
	}



	.stack (NOLOAD) :
	{
		. = __vec_offset - TOTAL_STACK_SIZE;
		__stack_start = .;

		. += MAIN_STACK_SIZE;
		. = ALIGN(8);
		__main_stack_top = .;

		. += PROCESS_STACK_SIZE;
		. = ALIGN(8);
		__process_stack_top = .;

		__stack_end = .;
	} >CONT_RAM


	.data :
	{
		. = ALIGN(4);
		__data_load = LOADADDR(.data);
		__data_start = .;
		*(.data .data.* .gnu.linkonce.d.*)
		. = ALIGN(4);
	} >CONT_RAM AT >FLASH

	.fastdata :
	{
		__fastdata_load = LOADADDR(.fastdata);
		__fastdata_start = .;
		*(.fastdata .fastdata.*)
		. = ALIGN(4);
		__fastdata_end = .;
	} >CONT_RAM AT >FLASH
	.fastcode :
	{
		__fastcode_load = LOADADDR(.fastcode);
		__fastcode_start = .;
		*(.fastcode .fastcode.*)
		. = ALIGN(4);
		__fastcode_end = .;
	} >CONT_RAM AT >FLASH
	.data_ram :
	{
		__data_ram_load = LOADADDR(.data_ram);
		__data_ram_start = .;
		*(.data_ram .data_ram.*)
		. = ALIGN(4);
		__data_ram_end = .;
	} >CONT_RAM AT >FLASH
	__data_end = .;

	.bss (NOLOAD) :
	{
		__bss_start = . ;
		*(.bss .bss.* .gnu.linkonce.b.*)
		. = ALIGN(4);
		__bss_end = .;
	} >CONT_RAM

	.bss_ram (NOLOAD) :
	{
		__bss_ram_start = . ;
		*(.bss_ram .bss_ram.*)
		. = ALIGN(4);
		__bss_ram_end = .;
	} >CONT_RAM
	__bss_end = .;

	.noinit (NOLOAD) :
	{
		__noinit_start = .;
		*(.noinit .noinit.*)
		. = ALIGN(4);
		__noinit_end = .;
	} >CONT_RAM


	.noinit_ram (NOLOAD) :
	{
		. = ALIGN(4);
		__noinit_ram_start = .;
		*(.noinit_ram .noinit_ram.*)
		. = ALIGN(4);
		__noinit_ram_end = .;
	} >CONT_RAM
	.heap_ram (NOLOAD) :
	{
		. = ALIGN(4);
		__heap_ram_start = .;
		. = MAX(ABSOLUTE(.), ORIGIN(RAM) + LENGTH(RAM));
		__heap_ram_end = .;
	} >CONT_RAM


    


	.rodata :
	{
		. = ALIGN(4);
		__table_zero_intern_start = .;
		LONG(__bss_start)
		LONG(__bss_end)
		__table_zero_intern_end = .;

		__table_copy_intern_start = .;
		LONG(__data_load)
		LONG(__data_start)
		LONG(__data_end)
		__table_copy_intern_end = .;

		__table_zero_extern_start = .;
        
		__table_zero_extern_end = .;
		__table_copy_extern_start = .;
        
		__table_copy_extern_end = .;

		/* SRAM properties bit mask (16-bit):
		 *
		 * - 0x0001: accessible via S-Bus
		 * - 0x0002: accessible via D-Bus
		 * - 0x0004: accessible via I-Bus
		 * - 0x0008: accessible via DMA
		 * - 0x0010: accessible via DMA2D
		 *
		 * - 0x1FE0: reserved
		 *
		 * - 0x2000: Fast memory (Core- or Tightly-Coupled)
		 * - 0x4000: non-volatile (or battery backed) memory
		 * - 0x8000: external memory
		 */
		__table_heap_start = .;
		LONG(0x001f)
		LONG(__heap_ram_start)
		LONG(__heap_ram_end)
        
		__table_heap_end = .;

	} >FLASH


	. = ALIGN(4);
	__rom_end = .;


	/* DWARF debug sections */
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_info     0 : { *(.debug_info) }
	.debug_line     0 : { *(.debug_line) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	.debug_pubtypes 0 : { *(.debug_pubtypes) }
	.debug_ranges   0 : { *(.debug_ranges) }
	.debug_str      0 : { *(.debug_str) }

	.comment 0 : { *(.comment) }
	.ARM.attributes 0 : { KEEP(*(.ARM.attributes)) }
	/DISCARD/ : { *(.note.GNU-stack)  }

}