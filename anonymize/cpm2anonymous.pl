#!/usr/bin/perl -w

# ##############################################################################
# this script gets a regular XML file in CPM format and replaces all sensible
# information with 'x'. This should help to debug any problems which might
# occure due to file size or content problems.
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
    my ($line);

    while (defined($line = <STDIN>))
      {
        $line =~ s/( label)="([^"]+)"/translateAttribute($1, $2);/ge;
        $line =~ s/>([^<]+)</translateCdata($1);/ge;

        print $line;
      }
  }


# ##############################################################################
sub translateAttribute
  {
    my ($label, $value) = @_;

    $value =~ s/./x/g;

    return $label . '="' . $value . '"';
  }


# ##############################################################################
sub translateCdata
  {
    my ($cdata) = @_;

    $cdata =~ s/./x/g;

    return '>' . $cdata . '<';
  }


# ##############################################################################

main();


# ##############################################################################

