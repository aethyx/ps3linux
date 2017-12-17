#BEGIN_CONFIG



echo "Configuring for x86..."

cat >> confdefs.h <<\EOF
#define TARGET_ILP32 1
EOF

cat >> confdefs.h <<\EOF
#define CACHE_LINE_SIZE 32
EOF


LIBDIRS=x86

# These are used for vpath and -I
ISA=x86
CPU_GRP=.
CPU_CORE=.

HOST_CPPDIRS="-I\$(top_srcdir)/include/x86 ${HOST_CPPDIRS}"
HOST_BFDNAME=elf32-i386
HOST_BFDARCH=i386
HOST_LDFLAGS="${HOST_LDFLAGS} -L\$(top_builddir)/lib/x86"
plat_outputs="${plat_outputs} lib/x86/Makefile"


if test "$host" = "$build"; then
  HOST_TOOLS_PREFIX=
else
  HOST_TOOLS_PREFIX=i386-linux-
fi

echo $ac_n "checking for supported boot-env for x86""... $ac_c" 1>&6
echo "configure:664: checking for supported boot-env for x86" >&5
case "${boot_env}" in
  default | grub | GRUB)
    boot_env=grub
    BOOT_ENV=boot_grub.o
    ;;
  *)
    { echo "configure: error: x86 Boot Environment ${boot_env}: unsupported" 1>&2; exit 1; }
esac
echo "$ac_t""$boot_env" 1>&6

echo $ac_n "checking for supported boot-console for x86""... $ac_c" 1>&6
echo "configure:676: checking for supported boot-console for x86" >&5
case "${boot_console_dev}" in
  default | UART | uart | UART-NS1675 | uart-ns1675)
    boot_console_dev=uart-ns1675
    IO_CHANNELS="${IO_CHANNELS} uartNS16750.o"
    BOOT_CONSOLE_DEV=boot_uart_ns16750.o
    if test "$boot_console_dev_addr" = "default"; then
      boot_console_dev_addr=0x3f8
    fi
    cat >> confdefs.h <<EOF
#define BOOT_CONSOLE_DEV_ADDR $boot_console_dev_addr
EOF

    if test "$boot_console_dev_opts" != "default"; then
      cat >> confdefs.h <<EOF
#define BOOT_CONSOLE_DEV_OPTS "$boot_console_dev_opts"
EOF

    fi
    ;;

  VGA | vga)
    boot_console_dev=vga
    IO_CHANNELS="${IO_CHANNELS} vga.o"
    BOOT_CONSOLE_DEV=boot_vga.o
    if test "$boot_console_dev_addr" = "default"; then
      boot_console_dev_addr=0xb8000
    fi
    cat >> confdefs.h <<EOF
#define BOOT_CONSOLE_DEV_ADDR $boot_console_dev_addr
EOF

    ;;

  *)
    { echo "configure: error: Unknown boot console: $boot_console" 1>&2; exit 1; }
    ;;
esac
echo "$ac_t""$boot_console_dev" 1>&6

if test "$enable_vga" = "yes"; then
  IO_CHANNELS="${IO_CHANNELS} vga.o"
fi

#END_CONFIG
