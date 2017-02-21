/*******************************************************************************
 * (c) Copyright 2011 Microsemi Corporation.  All rights reserved.
 *
 * SmartFusion/Cortex-M3 linker script for creating a production
 * image executing in SmartFusion internal eNVM.
 * Refer SmartFusion: Building Executable Image in Release Mode and Loading into eNVM Tutorial
 * at the following location 
 * http://www.actel.com/documents/SmartFusion_Release_Built_Tutorial.pdf
 *
 * Date : Tue, 27 Oct 2015  09:47:16
 */
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
	      "elf32-littlearm")
GROUP(-lc -lgcc -lm)
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)
SEARCH_DIR(.)
__DYNAMIC  =  0;

/*******************************************************************************
 * Start of board customization.
 *******************************************************************************/
MEMORY
{

  /* SmartFusion internal eSRAM */
  ram1 (rwx) : ORIGIN = 0x2000A000, LENGTH = 20472

  /* SmartFusion internal eNVM */
  rom1 (rx) : ORIGIN = 0x20000000, LENGTH = 40k

}
RAM_START_ADDRESS    = 0x2000A000; /* Must be the same value MEMORY region ram ORIGIN above. */
RAM_SIZE             = 20472;      /* Must be the same value MEMORY region ram LENGTH above. */
MAIN_STACK_SIZE      = 4k;         /* Cortex main stack size. */
PROCESS_STACK_SIZE   = 0k;          /* Cortex process stack size (only available with OS extensions).*/

/*******************************************************************************
 * End of board customization.
 *******************************************************************************/

PROVIDE (__main_stack_start = RAM_START_ADDRESS + RAM_SIZE);
PROVIDE (__process_stack_start = __main_stack_start - MAIN_STACK_SIZE);
PROVIDE (_estack = __main_stack_start);
PROVIDE (__mirrored_nvm = 0);   /* Indicate to startup code that NVM is not mirrored to VMA address .text copy is required. */

SECTIONS
{
  .reset :
  {
    *(.isr_vector)
    *sys_boot.o(.text)
    . = ALIGN(0x4);
  } > rom1

  .text :
  {
    CREATE_OBJECT_SYMBOLS
    __text_load = LOADADDR(.text);
    __text_start = .;

    *(.text .text.* .gnu.linkonce.t.*)
    *(.plt)
    *(.gnu.warning)
    *(.glue_7t) *(.glue_7) *(.vfp11_veneer)

    . = ALIGN(0x4);
    /* These are for running static constructors and destructors under ELF.  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*crtend.o(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*crtend.o(.dtors))

    *(.rodata .rodata.* .gnu.linkonce.r.*)

    *(.ARM.extab* .gnu.linkonce.armextab.*)
    *(.gcc_except_table)
    *(.eh_frame_hdr)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(.fini_array))
    KEEP (*(SORT(.fini_array.*)))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >rom1
 
  /* .ARM.exidx is sorted, so has to go in its own output section.  */
   __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } >rom1

  __exidx_end = .;
  _etext = .;

  .data :
  {
    __data_load = LOADADDR(.data);
    _sidata = LOADADDR (.data);
    __data_start = .;
    _sdata = .;
    KEEP(*(.jcr))
    *(.got.plt) *(.got)
    *(.shdata)
    *(.data .data.* .gnu.linkonce.d.*)
    . = ALIGN (4);
  	_edata = .;
  } >ram1 AT>rom1

  .bss :
  {
    __bss_start__ = . ;
    _sbss = .;
    *(.shbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN (8);
    __bss_end__ = .;
    _end = .;
    __end = _end;
    _ebss = .;
    PROVIDE(end = .);
  } >ram1 AT>rom1

  .stab 0 (NOLOAD) :
  {
    *(.stab)
  }

  .stabstr 0 (NOLOAD) :
  {
    *(.stabstr)
  }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  .note.gnu.arm.ident 0 : { KEEP (*(.note.gnu.arm.ident)) }
  .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) }
  /DISCARD/ : { *(.note.GNU-stack)  }
}
