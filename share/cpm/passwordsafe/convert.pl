#!/usr/bin/perl -w

# ##############################################################################
# this script gets a tab separated list from password safe text file and
# creates the XML body structure for CPM
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

use strict;
use warnings;


# ##############################################################################
sub main
  {
    my ($fline, $line, %xml);

    $fline = '';
    while (defined($line = <STDIN>))
      {
        if ($line =~ m/\r\r\n$/)
          {   # the line is continued
            if ($fline eq '')
              { $fline .= $line; }
            else
              { $fline .= " $line"; }
          }
        else
          {   # we found a new line
            if ($fline eq '')
              { $fline .= $line; }
            else
              { $fline .= " $line"; }

            processLine($fline);
            $fline = '';
          }
      }

    processLine($fline);
  }


# ##############################################################################
sub processLine
  {
    my ($line) = @_;
    my (%data, @tmp);

    $line =~ s/[\r\n]//g;
    if ($line eq '')
      { return; }

    # we remove the extra space right before the closing quote
    $line =~ s/ "$/"/;
    @tmp = split(/\t/, $line);

    if ($tmp[0] =~ m/^(.+)\.([^\.]+)$/)
      {
        $data{'host'}    = $1;
        $data{'service'} = $2;
      }
    else
      {
        $data{'host'}    = $tmp[0];
        $data{'service'} = 'default';
      }

    $data{'user'}     = $tmp[1];
    $data{'ucomment'} = $tmp[3];
    $data{'password'} = $tmp[2];

    $data{'ucomment'} =~ s/^"(.*)"$/$1/;

    print sprintf("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s%s\n",
        $data{'host'},
        "",
        $data{'service'},
        "",
        $data{'user'},
        $data{'ucomment'},
        $data{'password'},
        "",
        ";");
  }


# ##############################################################################

main();


# ##############################################################################

