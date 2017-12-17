#BEGIN_CONFIG


echo "Configuring for powerpc 64-bit ..."
echo "First platform adaption"

#
# Sanity checks
#
if test "$HYPE_IN_CONFIG" != "powerpc"; then
  { echo "configure: error: Must be run by the powerpc generic configure" 1>&2; exit 1; }
fi

case "${PLATFORM}" in
  ppc64*)
    HYPE_IN_CONFIG=powerpc64
    ;;
  *)
    { echo "configure: error: ${PLATFORM}: Not A Supported Platform!" 1>&2; exit 1; }
    ;;
esac

#
# these should apply to all powerpc64
#
cat >> confdefs.h <<\EOF
#define TARGET_LP64 1
EOF

cat >> confdefs.h <<\EOF
#define HAS_64BIT 1
EOF

cat >> confdefs.h <<\EOF
#define HAS_HTAB 1
EOF

cat >> confdefs.h <<\EOF
#define HAS_FP 1
EOF

cat >> confdefs.h <<\EOF
#define NUM_SPRGS 4
EOF

cat >> confdefs.h <<\EOF
#define HAS_MSR_SF 1
EOF


cat >> confdefs.h <<\EOF
#define CACHE_LINE_SIZE 128
EOF


CPU_GRP=64

ccp_isa="-I\$(top_srcdir)/include/\$(ISA)"
ccp_cpu_grp="-I\$(top_srcdir)/include/\$(ISA)/\$(CPU_GRP)"
ccp_cpu_core="-I\$(top_srcdir)/include/\$(ISA)/\$(CPU_GRP)/\$(CPU_CORE)"
HOST_CPPDIRS="${ccp_cpu_core} ${ccp_cpu_grp} ${ccp_isa} ${HOST_CPPDIRS}"

HOST_BFDNAME=elf64-powerpc
HOST_BFDARCH=powerpc
plat_outputs="${plat_outputs} lib/powerpc/64/Makefile"

echo "Second platform adaption"

case "${PLATFORM}" in
  ppc64-970*-*)
    # not sure if this is true
    cat >> confdefs.h <<\EOF
#define HAS_MSR_IP 1
EOF
 

    # HAS_RMOR is actually an misnomer, instead it 
    # should be called HAS_HYPERVISOR
    # power4 has one.
    cat >> confdefs.h <<\EOF
#define HAS_RMOR 1
EOF


    cat >> confdefs.h <<\EOF
#define HAS_SWSLB 1
EOF

    cat >> confdefs.h <<\EOF
#define SWSLB_SR_NUM 64
EOF
 # how many SLB's to save and restore
    cat >> confdefs.h <<\EOF
#define HAS_HYPE_SYSCALL 1
EOF

    cat >> confdefs.h <<\EOF
#define HAS_HDECR 1
EOF

    cat >> confdefs.h <<\EOF
#define HAS_MSR_HV 1
EOF

    cat >> confdefs.h <<\EOF
#define CPU_CORE 970
EOF

    machine_name_string="Momentum,Maple"
    cat >> confdefs.h <<EOF
#define MACHINE_NAME_STRING "${machine_name_string}"
EOF

    CPU_CORE=970

    # defaults
    enable_openfw=yes
    with_eic=openpic

    case "${PLATFORM}" in
      ppc64-970*-maple)
	CUSTOM_HW=maple_hw.o
        IO_XLATE=io_xlate_u3.o
	MACHINE=maple
        ;;
      ppc64-970*-JS20 | ppc64-970*-js20)
	CUSTOM_HW=js20_hw.o
        IO_XLATE=io_xlate_u3.o
	MACHINE=js20
        ;;
      ppc64-970*-mambo)
        CUSTOM_HW=mambo_hw.o
	MACHINE=mambo
        ;;
      ppc64-970*-metal)
        HW_QUIRKS=none
	enable_openfw=no
	MACHINE=metal
        ;;
      ppc64-970*)
        HW_QUIRKS=none
	echo "configure: warning: There is no 970 machine defined." 1>&2
        ;;
    esac
    ;;
  ppc64-*)
    script=`echo "${PLATFORM}" | sed -e 's/ppc64-//'`
    script=`echo "${script}" | sed -e 's/-.*//'`
    if test -r "${srcdir}/${script}/configure.frag"; then
      . ${srcdir}/${script}/configure.frag
    else
      { echo "configure: error: ${PLATFORM}: Not A Supported Platform!" 1>&2; exit 1; }
    fi
    ;;
  *)
    { echo "configure: error: ${PLATFORM}: Not A Supported Platform!" 1>&2; exit 1; }
    ;;
esac

echo "Platform adaptions done"

HOST_LDFLAGS="${HOST_LDFLAGS} -L\$(top_builddir)/lib/\$(ISA)/\$(CPU_GRP)"

if test "${enable_openfw}" = "yes"; then
   of_opt_outputs="${of_opt_outputs} plugins/openfw/powerpc/64/Makefile"
fi


if test "$host" = "$build"; then
  HOST_TOOLS_PREFIX=
else
  HOST_TOOLS_PREFIX=powerpc64-linux-
fi

HOST_ALT_TOOLS=ppc32
HOST_ALT_BFDNAME=elf32-powerpc

case "$build" in
  powerpc*-*linux-*)
    HOST_ALT_TOOLS_PREFIX=
    ;;
  *)
    HOST_ALT_TOOLS_PREFIX=powerpc-linux-
    ;;
esac

case "$with_hba" in
  default)
    HOST_BUS_ADAPTER=hba_default.o
    ;;
  rtas)
    HV_USES_RTAS=yes;
    HOST_BUS_ADAPTER=hba_rtas.o
    with_hba=rtas
    ;;
  *)
    echo "configure: warning: HBA: ${with_hba}: not supported." 1>&2
    { echo "configure: error: ${PLATFORM} supports rtas or default." 1>&2; exit 1; }
    ;;
esac


#END_CONFIG
