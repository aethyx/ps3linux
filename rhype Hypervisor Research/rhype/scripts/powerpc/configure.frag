#BEGIN_CONFIG


echo "Configuring for powerpc..."

if test "$HYPE_IN_CONFIG" != "toplevel"; then
  { echo "configure: error: Must be sourced by the toplevel configure script" 1>&2; exit 1; }
fi

HYPE_IN_CONFIG=powerpc

#
# This tells the <srcdir>/lib Makefiles what subdirs to go into
#
LIBDIRS=powerpc
ISA=powerpc

plat_outputs="${plat_outputs} lib/powerpc/Makefile"
# always do 32
plat_outputs="${plat_outputs} lib/powerpc/32/Makefile"

echo $ac_n "checking for supported boot-env for PPC""... $ac_c" 1>&6
echo "configure:653: checking for supported boot-env for PPC" >&5
case "${boot_env}" in
  default | OF | of)
    boot_env=of
    BOOT_ENV=boot_of.o
    BOOT_ENVIRONMENT=OF
    cat >> confdefs.h <<\EOF
#define BOOT_ENVIRONMENT_of 1
EOF

    ;;
  metal)
    boot_env=metal
    BOOT_ENV=boot_metal.o
    BOOT_ENVIRONMENT=Metal
    cat >> confdefs.h <<\EOF
#define BOOT_ENVIRONMENT_metal 1
EOF

    ;;
  *)
    { echo "configure: error: PPC Boot Environment ${boot_env}: unsupported" 1>&2; exit 1; }
esac
echo "$ac_t""$boot_env" 1>&6

echo $ac_n "checking for supported boot-console for PPC""... $ac_c" 1>&6
echo "configure:679: checking for supported boot-console for PPC" >&5
case "${boot_console_dev}" in
  default | OF | of)
    boot_console_dev=of
    IO_CHANNELS="${IO_CHANNELS}" # io_chan_of.o"
    BOOT_CONSOLE_DEV=boot_of.o
    cat >> confdefs.h <<\EOF
#define BOOT_CONSOLE_of 1
EOF

    ;;
  Zilog | zilog )
    boot_console_dev=zilog;
    IO_CHANNELS="${IO_CHANNELS} zilog.o"
    if test "$boot_console_dev_addr" != "default"; then
      cat >> confdefs.h <<EOF
#define BOOT_CONSOLE_DEV_ADDR $boot_console_dev_addr
EOF

    fi
    cat >> confdefs.h <<\EOF
#define BOOT_CONSOLE_zilog 1
EOF

    
    ;;
  VGA | vga)
    boot_console_dev=vga
    IO_CHANNELS="${IO_CHANNELS} vga.o"
    BOOT_CONSOLE_DEV=boot_vga.o
    if test "$boot_console_dev_addr" != "default"; then
      cat >> confdefs.h <<EOF
#define BOOT_CONSOLE_DEV_ADDR $boot_console_dev_addr
EOF

    fi
    cat >> confdefs.h <<\EOF
#define BOOT_CONSOLE_vga 1
EOF

    ;;

  UART | uart | UART-NS1675 | uart-ns1675)
    boot_console_dev=uart_ns1675
    IO_CHANNELS="${IO_CHANNELS} uartNS16750.o"
    BOOT_CONSOLE_DEV=boot_uart_ns16750.o
    if test "$boot_console_dev_addr" != "default"; then
      cat >> confdefs.h <<EOF
#define BOOT_CONSOLE_DEV_ADDR $boot_console_dev_addr
EOF

    fi
    if test "$boot_console_dev_opts" != "default"; then
      cat >> confdefs.h <<EOF
#define BOOT_CONSOLE_DEV_OPTS "$boot_console_dev_opts"
EOF

    fi
    cat >> confdefs.h <<\EOF
#define BOOT_CONSOLE_uart_ns1675 1
EOF

    ;;

  *)
    { echo "configure: error: Unknown boot console: $boot_console_dev: " 1>&2; exit 1; }
    ;;
esac
echo "$ac_t""$boot_console_dev" 1>&6

cat >> confdefs.h <<EOF
#define BOOT_CONSOLE_DEV BOOT_CONSOLE_$boot_console_dev
EOF


IO_CHANNELS="${IO_CHANNELS} zilog.o"

echo $ac_n "checking for default output device OF-path""... $ac_c" 1>&6
echo "configure:757: checking for default output device OF-path" >&5
# Check whether --with-of-output-device or --without-of-output-device was given.
if test "${with_of_output_device+set}" = set; then
  withval="$with_of_output_device"
    of_output_device=$withval;
     cat >> confdefs.h <<EOF
#define OF_OUTPUT_DEVICE "$of_output_device"
EOF
 
fi


echo "$ac_t""$of_output_device" 1>&6

machine_name_string="PowerPC-System"
cat >> confdefs.h <<EOF
#define MACHINE_NAME_STRING "${machine_name_string}"
EOF


case "${PLATFORM}" in
  ppc64*)
    . ${srcdir}/scripts/powerpc/64/configure.frag
    ;;
  ppc32*)
    . ${srcdir}/scripts/powerpc/32/configure.frag
    ;;
  *)
    { echo "configure: error: ${PLATFORM}: Not A Supported Platform!" 1>&2; exit 1; }
    ;;
esac

if test "${enable_openfw}" = "yes"; then
   of_opt_outputs="plugins/openfw/powerpc/Makefile ${of_opt_outputs}"
   of_opt_outputs="${of_opt_outputs} plugins/openfw/powerpc/32/Makefile"
fi

echo $ac_n "checking if hypervisor is using rtas""... $ac_c" 1>&6
echo "configure:795: checking if hypervisor is using rtas" >&5
if test "x$HV_USES_RTAS" = "x"; then
   HV_USES_RTAS=no
else
   cat >> confdefs.h <<\EOF
#define HV_USES_RTAS 1
EOF

fi
echo "$ac_t""$HV_USES_RTAS" 1>&6


#END_CONFIG
