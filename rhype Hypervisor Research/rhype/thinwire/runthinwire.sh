#!/bin/sh
# Copyright (C) 2005 Jimi Xenidis <jimix@watson.ibm.com>, IBM Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# $Id$
#

if [ -z "$interp" ]; then
    #
    # Need to make sure we are running bash.
    # We also want to time the script.
    #
    interp=yes
    export interp
    exec sh -c "bash $0 $*"
fi

##set -x

prog=$0

usage() {
    cat - <<EOF
usage: $prog [options]
  -h	     Show this message and exit.
  -n         Skip reading the .thinwirerc in the HOME directory.
  -s         Run SimIP (envar: run_simip).
  -i	     No Input so merge all output (EXPERIMENTAL!)
  -r	     Launch consoles in raw (no line buffering) mode.
  -x	     Do not launch xterms, will output commands to put in your
             owns shell.
  -d <tty>   Use a tty/pty for host communication.
  -X <prog>  Use this <prog> instead of xterm to launch shells.
  -m <host>  Host running mambo (default = localhost, envar: mambo_host).
  -p <n>     Number of partitions (default = 1, envar: partition).
  -b <port>  TCP port number to use as base (default = 2100, envar base_port).
  -u <name>  Use Unix domain socket for host communication.
  -T <opts>  Pass these options on to thinwire.
EOF
}

readrc() {
  while [ -n "$1" ]; do
    if [ "$1" = "-n" ]; then
      return 0
    fi
  done
  test -r $HOME/.thinwirerc && source $HOME/.thinwirerc
}

prepend() {
  local pre=$1
  IFS=''
  while read line; do
    echo "[${pre}]$line"
  done
}

#
# Defaults
#
mambo_host=localhost
partitions=1
base_port=2101
port_group=8
span=$[ ${port_group} - 1 ]
run_simip=no
xterm='xterm -tm intr^t'
run_xterm=yes
no_input=no
tty=no
raw_mode="-noraw"
thinwire_options=

readrc

#
# Process Options
#
while getopts "nihsxrd:X:m:p:b:u:T:" Option; do
  case $Option in
  h)  usage                  # h -> help
      exit
      ;;
  s)  run_simip=yes
      ;;
  i)  no_input=yes
      set -o ignoreeof
      ;;
  n)  ;;	# check for this one done above
  x)  run_xterm=no
      ;;
  r)  raw_mode=""
      ;;
  u)  if test ! -S "$OPTARG" ; then
	echo "${prog}: Not a Unix domain socket: $OPTARG"
	usage
	exit 1
      fi
      if test ! -w "$OPTARG" ; then
	echo "${prog}: Not writable: $OPTARG"
	usage
	exit 1
      fi
      tty=$OPTARG
      ;;
  d)  if test ! -c "$OPTARG" ; then
	echo "${prog}: Not a character device: $OPTARG"
	usage
	exit 1
      fi
      if test ! -w "$OPTARG" ; then
	echo "${prog}: Not writable: $OPTARG"
	usage
	exit 1
      fi
      tty=$OPTARG
      ;;
  X)  if [ -z "$(type -t $OPTARG)" ]; then
	echo "${prog}: command not found: $OPTARG"
	usage
	exit 1
      fi
      xterm=$OPTARG
      ;;
  m)  mambo_host=$OPTARG
      ;;
  p)  partitions=$OPTARG
      ;;
  b)  base_port=$OPTARG
      ;;
  T)  thinwire_options="$OPTARG"
      ;;

  *)  echo "Illegal option: $Option"
      usage
      exit 1
      ;;
  esac
done

thisdir=$(dirname $0)
if [ -x ${thisdir}/thinwire3 ]; then
  execdir=${thisdir}
else
  ## Thinwire executable is not in this directory, it must be in path then
  tw=$(type -p thinwire)
  if [ -d "${tw}" ]; then
    execdir=$(dirname "${tw}")
  else
    echo "$prog: thinwire executable could not be found."
    echo "$prog:  please make sure it is in your path or in: ${thisdir}"
    exit 1
  fi
fi

thinwire=${execdir}/thinwire3
console=${execdir}/console
simip=${execdir}/simip

test  ! -x "${thinwire}" && echo "$prog: ${thinwire}: not found." && exit 1
test  ! -x "${console}"  && echo "$prog: ${console}: not found." && exit 1
test  ! -x "${simip}"    && echo "$prog: ${simip}: not found." && exit 1

console="${console} ${raw_mode}"

# master Thinwire port 
master=$[	${base_port} + 0]
master="${mambo_host}:${master}"
if [ "$tty" != "no" ]; then
  master=$tty
fi

if [ "$partitions" -eq 1 ]; then
  span=32
fi

# Hypervisor Console
hcons=$[	${base_port} + 1]
#hcons=stdout
# Reserved for use by hypervisor level GDB
hgdb=$[		${base_port} + 2]
# unused
hport3=$[	${base_port} + 3]

mambo_ports="${master} ${hcons} ${hgdb} ${hport3}"

thin_channels() {
  local port=$1
  local count=$2
  local last=$[$port + $count]
  local cports=""

  while [ ${port} -lt ${last} ]; do
    cports="${cports} ${port}"
    port=$[${port} + 1]
  done
  echo ${cports}
}


thin_console_ports() {
  local p=$1
  local port=$2
  local cports=""
  while [ ${p} -gt 0 ]; do
    port=$[ ${port} + ${port_group} ]
    cports="${cports} : $(thin_channels ${port} ${span})"
    p=$[ ${p} - 1 ]
  done
  echo $cports
}
console_ports=$(thin_console_ports ${partitions} ${base_port})

${thinwire} ${thinwire_options} ${mambo_ports} ${console_ports}  &
thinwire_pid=$!

simip_pid=
if [ ${run_simip} = "yes" ]; then
  ${simip} localhost:${simip_com} localhost:${simip_poll} &
  simip_pid=$!
fi

make_console_ports() {
  local p=$1
  local port=$2
  local host=$3
  local cports=""
  while [ ${p} -gt 0 ]; do
    port=$[ ${port} + ${port_group} ]
    cports="${cports} ${host}:${port}"
    p=$[ ${p} - 1 ]
  done
  echo $cports
}
console_ports=$(make_console_ports ${partitions} ${base_port} localhost)

cons_pids=
pnum=0
for p in ${console_ports}; do
  if [ "${no_input}" = yes ]; then
    pnum=$[pnum + 1]
    ((${console} $p 2>&1 < /dev/tty) | prepend p${pnum}) &
    test $! != $$ && cons_pids="${cons_pids} $!"
  elif [ "${run_xterm}" = "yes" ]; then
    ${xterm} -T "console [$p]" -e ${console} $p &
    test $! != $$ && cons_pids="${cons_pids} $!"
  else
    echo ${console} $p
  fi
done

trap "kill $thinwire_pid $simip_pid $cons_pids" HUP INT QUIT

# Run the Hypervisor's console in this shell
(${console} localhost:${hcons})

kill $thinwire_pid $simip_pid $cons_pids
