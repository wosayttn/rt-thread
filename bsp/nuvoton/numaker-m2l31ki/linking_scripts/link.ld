PROVIDE(__STACK_SIZE = 0x00001000);
PROVIDE(__HEAP_SIZE  = 0x00000000);

/* System memory brief */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K    /* 512K */
  SRAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 160K    /* 160K */
}

PROVIDE(__sram_start__ = ORIGIN(SRAM));
PROVIDE(__sram_end__ = ORIGIN(SRAM) + LENGTH(SRAM));

ENTRY(Reset_Handler)

SECTIONS
{
  .text :
  {
    __Vectors = .;
    KEEP(*(.vectors))
    *startup_*.o(.text*)

    *(.text*)

    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
    *crtbegin.o(.dtors)
    *crtbegin?.o(.dtors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
    *(SORT(.dtors.*))
    *(.dtors)

    /* section information for finsh shell */
    . = ALIGN(4);
    __fsymtab_start = .;
    KEEP(*(FSymTab))
    __fsymtab_end = .;
    . = ALIGN(4);
    __vsymtab_start = .;
    KEEP(*(VSymTab))
    __vsymtab_end = .;
    . = ALIGN(4);

    /* section information for initial. */
    . = ALIGN(4);
    __rt_init_start = .;
    KEEP(*(SORT(.rti_fn*)))
    __rt_init_end = .;
    . = ALIGN(4);

    /* section information for utest */
    . = ALIGN(4);
    __rt_utest_tc_tab_start = .;
    KEEP(*(UtestTcTab))
    __rt_utest_tc_tab_end = .;

    /* section information for cherryusb usb host class */
    . = ALIGN(4);
    __usbh_class_info_start__ = .;
    KEEP(*(.usbh_class_info))
    __usbh_class_info_end__ = .;

    *(.ctors)

    KEEP(*(.eh_frame*))
  } > FLASH

  .rodata : {
    . = ALIGN(8);
    *(.rodata*)
    . = ALIGN(4);
    __zero_table_start__ = .;
    LONG (__bss_start__)
    LONG ((__bss_end__ - __bss_start__) / 4)
    __zero_table_end__ = .;

    . = ALIGN(4);
    __copy_table_start__ = .;
    LONG (__etext)
    LONG (__data_start__)
    LONG ((__data_end__ - __data_start__) / 4)
    __copy_table_end__ = .;
    . = ALIGN(8);
  } > FLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } > FLASH

  __etext = ALIGN (4);

  .stack (ORIGIN(SRAM)) (NOLOAD) :
  {
    . = ALIGN(8);
    __STACK_LIMIT = .;
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __INITIAL_SP = .;
    __StackTop = .;
  } > SRAM
  PROVIDE(__stack = __INITIAL_SP);

  .heap (__INITIAL_SP) (NOLOAD) :
  {
    . = ALIGN(8);
    __end__ = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > SRAM

  .data __HeapLimit : AT (__etext)
  {
    __data_start__ = .;
    *(vtable)
    *(.data)
    *(.data.*)

    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    PROVIDE(__ctors_start__ = .);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);
    PROVIDE(__ctors_end__ = .);

    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(4);
    __data_end__ = .;
  } > SRAM

  .bss :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > SRAM AT > SRAM
}
