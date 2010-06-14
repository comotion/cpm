#!/bin/bash

# ##############################################################################
# this script gets called by all importers and adds the bounding XML structure
# ##############################################################################
# Copyright (C) 2005-2009 Harry Brueckner
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

if [ ${#} != 0 ]; then
  echo "syntax: ${0}"
  exit
fi


# ##############################################################################

TEMPFILE="/tmp/cpm_convert.tmp"

# ##############################################################################

(
  IFS="
"
  echo '<?xml version="1.0" encoding="ISO-8859-1"?>'
  echo '<root>'

  # here we simply read STDIN and add it to the XML frame
  while read LINE; do
    echo "${LINE}"
  done

  echo '</root>'
) | \
    tee ${TEMPFILE} | \
    gzip -9 | \
    gpg --armour --sign --encrypt > cpm_convert.db

xmllint --noout "${TEMPFILE}"
if [ "${?}" != 0 ]; then
  echo
  echo "ERROR"
  echo "The imported data does not validate properly. The created database"
  echo "can not be used without further, manual modification of the file."
fi

shred "${TEMPFILE}"
rm -fr "${TEMPFILE}"

echo
echo "a new cpm database has been created in cpm_convert.db"


# ##############################################################################

