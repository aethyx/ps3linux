#BEGIN_CONFIG


echo "Configuring for ppc32..."


PPC32_TOOLS=powerpc-linux-
case "${build}" in
  powerpc*-linux*) PPC32_TOOLS= ;;
esac


PPC32_TOOLS=powerpc-linux-
case "${build}" in
  powerpc-linux*) PPC32_TOOLS= ;;
esac

# Extract the first word of "${PPC32_TOOLS}ar", so it can be a program name with args.
set dummy ${PPC32_TOOLS}ar; ac_word=$2
echo $ac_n "checking for $ac_word""... $ac_c" 1>&6
echo "configure:651: checking for $ac_word" >&5
if eval "test \"`echo '$''{'ac_cv_path_PPC32_AR'+set}'`\" = set"; then
  echo $ac_n "(cached) $ac_c" 1>&6
else
  case "$PPC32_AR" in
  /*)
  ac_cv_path_PPC32_AR="$PPC32_AR" # Let the user override the test with a path.
  ;;
  ?:/*)			 
  ac_cv_path_PPC32_AR="$PPC32_AR" # Let the user override the test with a dos path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
  ac_dummy="$PATH:/opt/cross/bin"
  for ac_dir in $ac_dummy; do 
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      ac_cv_path_PPC32_AR="$ac_dir/$ac_word"
      break
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$ac_cv_path_PPC32_AR" && ac_cv_path_PPC32_AR="no"
  ;;
esac
fi
PPC32_AR="$ac_cv_path_PPC32_AR"
if test -n "$PPC32_AR"; then
  echo "$ac_t""$PPC32_AR" 1>&6
else
  echo "$ac_t""no" 1>&6
fi

if test "$PPC32_AR" = no; then
  { echo "configure: error: ${PPC32_TOOLS}ar was not found." 1>&2; exit 1; }
fi

# Extract the first word of "${PPC32_TOOLS}ranlib", so it can be a program name with args.
set dummy ${PPC32_TOOLS}ranlib; ac_word=$2
echo $ac_n "checking for $ac_word""... $ac_c" 1>&6
echo "configure:691: checking for $ac_word" >&5
if eval "test \"`echo '$''{'ac_cv_path_PPC32_RANLIB'+set}'`\" = set"; then
  echo $ac_n "(cached) $ac_c" 1>&6
else
  case "$PPC32_RANLIB" in
  /*)
  ac_cv_path_PPC32_RANLIB="$PPC32_RANLIB" # Let the user override the test with a path.
  ;;
  ?:/*)			 
  ac_cv_path_PPC32_RANLIB="$PPC32_RANLIB" # Let the user override the test with a dos path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
  ac_dummy="$PATH:/opt/cross/bin"
  for ac_dir in $ac_dummy; do 
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      ac_cv_path_PPC32_RANLIB="$ac_dir/$ac_word"
      break
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$ac_cv_path_PPC32_RANLIB" && ac_cv_path_PPC32_RANLIB="no"
  ;;
esac
fi
PPC32_RANLIB="$ac_cv_path_PPC32_RANLIB"
if test -n "$PPC32_RANLIB"; then
  echo "$ac_t""$PPC32_RANLIB" 1>&6
else
  echo "$ac_t""no" 1>&6
fi

if test "$PPC32_RANLIB" = no; then
  { echo "configure: error: ${PPC32_TOOLS}ranlib was not found." 1>&2; exit 1; }
fi

# Extract the first word of "${PPC32_TOOLS}gcc", so it can be a program name with args.
set dummy ${PPC32_TOOLS}gcc; ac_word=$2
echo $ac_n "checking for $ac_word""... $ac_c" 1>&6
echo "configure:731: checking for $ac_word" >&5
if eval "test \"`echo '$''{'ac_cv_path_PPC32_CC'+set}'`\" = set"; then
  echo $ac_n "(cached) $ac_c" 1>&6
else
  case "$PPC32_CC" in
  /*)
  ac_cv_path_PPC32_CC="$PPC32_CC" # Let the user override the test with a path.
  ;;
  ?:/*)			 
  ac_cv_path_PPC32_CC="$PPC32_CC" # Let the user override the test with a dos path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
  ac_dummy="$PATH:/opt/cross/bin"
  for ac_dir in $ac_dummy; do 
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      ac_cv_path_PPC32_CC="$ac_dir/$ac_word"
      break
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$ac_cv_path_PPC32_CC" && ac_cv_path_PPC32_CC="no"
  ;;
esac
fi
PPC32_CC="$ac_cv_path_PPC32_CC"
if test -n "$PPC32_CC"; then
  echo "$ac_t""$PPC32_CC" 1>&6
else
  echo "$ac_t""no" 1>&6
fi

if test "$PPC32_CC" = no; then
  { echo "configure: error: ${PPC32_TOOLS}gcc was not found." 1>&2; exit 1; }
fi

# Extract the first word of "${PPC32_TOOLS}objcopy", so it can be a program name with args.
set dummy ${PPC32_TOOLS}objcopy; ac_word=$2
echo $ac_n "checking for $ac_word""... $ac_c" 1>&6
echo "configure:771: checking for $ac_word" >&5
if eval "test \"`echo '$''{'ac_cv_path_PPC32_OBJCOPY'+set}'`\" = set"; then
  echo $ac_n "(cached) $ac_c" 1>&6
else
  case "$PPC32_OBJCOPY" in
  /*)
  ac_cv_path_PPC32_OBJCOPY="$PPC32_OBJCOPY" # Let the user override the test with a path.
  ;;
  ?:/*)			 
  ac_cv_path_PPC32_OBJCOPY="$PPC32_OBJCOPY" # Let the user override the test with a dos path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
  ac_dummy="$PATH:/opt/cross/bin"
  for ac_dir in $ac_dummy; do 
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      ac_cv_path_PPC32_OBJCOPY="$ac_dir/$ac_word"
      break
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$ac_cv_path_PPC32_OBJCOPY" && ac_cv_path_PPC32_OBJCOPY="no"
  ;;
esac
fi
PPC32_OBJCOPY="$ac_cv_path_PPC32_OBJCOPY"
if test -n "$PPC32_OBJCOPY"; then
  echo "$ac_t""$PPC32_OBJCOPY" 1>&6
else
  echo "$ac_t""no" 1>&6
fi

if test "$PPC32_OBJCOPY" = no; then 
  { echo "configure: error: ${PPC32_TOOLS}objcopy was not found." 1>&2; exit 1; }
fi

# Extract the first word of "${PPC32_TOOLS}objdump", so it can be a program name with args.
set dummy ${PPC32_TOOLS}objdump; ac_word=$2
echo $ac_n "checking for $ac_word""... $ac_c" 1>&6
echo "configure:811: checking for $ac_word" >&5
if eval "test \"`echo '$''{'ac_cv_path_PPC32_OBJDUMP'+set}'`\" = set"; then
  echo $ac_n "(cached) $ac_c" 1>&6
else
  case "$PPC32_OBJDUMP" in
  /*)
  ac_cv_path_PPC32_OBJDUMP="$PPC32_OBJDUMP" # Let the user override the test with a path.
  ;;
  ?:/*)			 
  ac_cv_path_PPC32_OBJDUMP="$PPC32_OBJDUMP" # Let the user override the test with a dos path.
  ;;
  *)
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS=":"
  ac_dummy="$PATH:/opt/cross/bin"
  for ac_dir in $ac_dummy; do 
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      ac_cv_path_PPC32_OBJDUMP="$ac_dir/$ac_word"
      break
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$ac_cv_path_PPC32_OBJDUMP" && ac_cv_path_PPC32_OBJDUMP="no"
  ;;
esac
fi
PPC32_OBJDUMP="$ac_cv_path_PPC32_OBJDUMP"
if test -n "$PPC32_OBJDUMP"; then
  echo "$ac_t""$PPC32_OBJDUMP" 1>&6
else
  echo "$ac_t""no" 1>&6
fi

if test "$PPC32_OBJDUMP" = no; then
  { echo "configure: error: ${PPC32_TOOLS}objdump was not found." 1>&2; exit 1; }
fi


echo $ac_n "checking with-platform""... $ac_c" 1>&6
echo "configure:850: checking with-platform" >&5
# Check whether --with-platform or --without-platform was given.
if test "${with_platform+set}" = set; then
  withval="$with_platform"
  PLATFORM="$withval"
fi

echo "$ac_t""$PLATFORM" 1>&6

case "$PLATFORM" in
  44x)
    cat >> confdefs.h <<\EOF
#define TARGET_ILP32 1
EOF

    cat >> confdefs.h <<\EOF
#define CPU_4xx 1
EOF

    cat >> confdefs.h <<\EOF
#define CPU_44x 1
EOF

    cat >> confdefs.h <<\EOF
#define NUM_SPRGS 8
EOF

    cat >> confdefs.h <<\EOF
#define CACHE_LINE_SIZE 32
EOF

    # not sure if this is true
    cat >> confdefs.h <<\EOF
#define HAS_MSR_IP 1
EOF


    ISA=ppc32

    ARCH_TOOLS=$PPC32_TOOLS
    PREBOOT=preboot.o
    BOOTENTRY="-e _preboot"
    ARCH_CFLAGS="-Wa,-mbooke,-m405"
    ARCH_CPPFLAGS="-I\$(top_srcdir)/include/44x ${ARCH_CPPFLAGS}"
    ARCH_LDFLAGS="-L\$(top_builddir)/lib/ppc32"
    ARCH_OBJS="vm_4xx.o tlb_4xx.o init_4xx.o"
    BOOT_OBJS="preboot.o init_4xx.o"
    LIBDIRS="ppc32"

#    gcc_incs=`${PPC32_CC} ${ARCH_CFLAGS} -print-file-name=include`

    ;;
  *)
    echo "$PLATFORM: Not A Supported Platform!"
    exit
    ;;
esac

#END_CONFIG
