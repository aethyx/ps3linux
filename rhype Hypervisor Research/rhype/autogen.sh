#! /bin/sh
#
# Make sure we are run by bash
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
#
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
#
#set -x
#
# Perhaps we were fed the right incantation for autoconf
#

export HYPE_AUTOGEN_RUNNING=yes

if [ -n "$1" ]; then
  a_conf=$(type -p $1)
  shift
else
  # list of known autoconf names for version 2.13, `autoconf' should be last.
  a_list="autoconf2.13 autoconf-2.13 autoconf"
  for ac in $a_list; do
    a_conf=$(type -p $ac)
    test -n "$a_conf" && break
  done
fi

if [ -z "$a_conf" ]; then
  echo "$0: cannot figure out how to run autoconf"
  exit 1
fi

# check for version of autoconf, We are not ready yet for 2.53
# but we should move to that at some point.  am guessing that
# 2.53 is the first version that does not work right yet

aver=$(${a_conf} --version | grep -i autoconf)
case $aver in
  [Aa]utoconf*2.13*)
    echo "$0: $aver: is known to be supported"
    ;;

  [Aa]utoconf*2.1[4-9]* | [Aa]utoconf*2.[234]*)
    echo "$0: $aver: is believed to be supported, be careful"
    ;;

  [Aa]utoconf*2.5[3-9]* | [Aa]utoconf*)
    echo "$0: $aver: is not supported yet please use version 2.13"
    exit 1
    ;;

  *)
    echo "$0: Problem with autoconf: $a_conf: $aver"
    exit 1
    ;;
esac

# Make sure I'm in the same directory as this script
cd `dirname $0`

list=`find * -name CVS -prune -o -name configure\*.in -print`

for c_in in $list; do
  d=`dirname $c_in`
  c=$d/`basename $c_in .in`

  if test "$c_in" -nt "$c"; then
    echo "processing: $c_in"

    # make it 
    test -e "$c" -a  ! -w "$c" && chmod +w $c

    # deal with the master normally
    case "$c_in" in
    configure.in|docs/configure.in)
      echo "autoconf $c_in > $c"
      $a_conf $c_in > $c
      ;;
    */configure.frag.in)
      echo "$a_conf $c_in | sed -ne '/#BEGIN_CONFIG/,/#END_CONFIG/p' > $c"
      $a_conf $c_in | sed -ne '/#BEGIN_CONFIG/,/#END_CONFIG/p' > $c
      ;;
    *)
      echo "unknown configure script: ${c_in}"
      exit 1;
    esac
  fi
    
done

echo done
