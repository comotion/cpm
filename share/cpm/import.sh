#!/bin/bash

# ##############################################################################
# this script calls the specified conversion script and adds the additional
# XML structure and encryption.
# ##############################################################################
# Copyright (C) 2005, 2006 Harry Brueckner
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or any later version.
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
#
# Contact: Harry Brueckner <harry_b@mm.st>
#          Muenchener Strasse 12a
#          85253 Kleinberghofen
#          Germany
# ##############################################################################


# ##############################################################################

if [ ${#} == 0 ]; then
  echo "syntax: ${0} [general|passwordsafe|pms]" 1>&2
  exit
fi


# ##############################################################################

PROCEDURE="${1}"
shift
set -- ${*}

BASE=`dirname ${0}`

case "${PROCEDURE}" in
  general)
      ${BASE}/general/convert.sh ${*} | \
          ${BASE}/csv2cpm.pl | \
          ${BASE}/convert2cpm.sh
      ;;
  passwordsafe)
      ${BASE}/general/convert.sh ${*} | \
          ${BASE}/passwordsafe/convert.pl | \
          ${BASE}/csv2cpm.pl | \
          ${BASE}/convert2cpm.sh
      ;;
  pms)
      ${BASE}/pms/convert.sh ${*} | \
          ${BASE}/pms/convert.pl | \
          ${BASE}/csv2cpm.pl | \
          ${BASE}/convert2cpm.sh
      ;;
  *)
      echo "${PROCEDURE} is an unknown import function" 1>&2
      exit 1
      ;;
esac


# ##############################################################################

