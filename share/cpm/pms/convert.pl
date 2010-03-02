#!/usr/bin/perl -w

# ##############################################################################
# this script gets a tab separated list from pms_export and creates the XML
# body structure for CPM
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

use strict;
use warnings;


# ##############################################################################
sub main
  {
    my ($line);
    my (%data);

    while (defined($line = <STDIN>))
      {
        my (@tmp);

        $line =~ s/[\r\n]//g;
        @tmp = split(/\t/, $line);

        $data{'host'}     = $tmp[0];
        $data{'hcomment'} = $tmp[1];
        $data{'user'}     = $tmp[2];
        $data{'ucomment'} = $tmp[3];
        $data{'password'} = $tmp[4];

        print sprintf("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s%s\n",
            $data{'host'},
            $data{'hcomment'},
            "default",
            "",
            $data{'user'},
            $data{'ucomment'},
            $data{'password'},
            "",
            ";");
      }
  }


# ##############################################################################

main();


# ##############################################################################

