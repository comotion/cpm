#!/usr/bin/perl -w

# ##############################################################################
# this script creates test-xml output which can be used to validate libxml and
# the data handling in the application.
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
use Getopt::Long;
use Time::localtime;


# ##############################################################################
@main::alphabet = ();

$main::config{'alphabet'}  = 'alphabet-ascii.txt';
$main::config{'depth-min'} = '1';
$main::config{'depth-max'} = '4';
$main::config{'nodes-min'} = '3';
$main::config{'nodes-max'} = '5';
$main::config{'text-min'}  = '10';
$main::config{'text-max'}  = '128';
$main::config{'users-min'} = '3';
$main::config{'users-max'} = '20';

$main::runtime{'help'} = 0;
$main::runtime{'users'} = 0;


# ##############################################################################
#
# Description     main function of the script which reads any commandline
#                 arguments and starts the XML output
# Author          Harry Brueckner
# Date            2005-06-01
# Arguments       void
# Return          void
#
sub main
  {
    my ($lmax, $users);


    if (!GetOptions(
          'alphabet=s'  => \$main::config{'alphabet'},
          'depth-min=i' => \$main::config{'depth-min'},
          'depth-max=i' => \$main::config{'depth-max'},
          'nodes-min=i' => \$main::config{'nodes-min'},
          'nodes-max=i' => \$main::config{'nodes-max'},
          'text-min=i'  => \$main::config{'text-min'},
          'text-max=i'  => \$main::config{'text-max'},
          'users-min=i' => \$main::config{'users-min'},
          'users-max=i' => \$main::config{'users-max'},
          ))
      { $main::runtime{'help'} = 1; }

    if ($main::runtime{'help'})
      {
        print STDERR <<EOF;
syntax: $0 --alphabet=FILE --depth-min=NR --depth-max=NR
                     --nodes-min=NR --nodes-max=NR
                     --text-min=NR --text-max=NR
                     --users-min=NR --users-max=NR

    --alphabet      specify the alphabet file
    --depth-min     minimum node depth
    --depth-max     maximum node depth
    --nodes-min     minimum sub-nodes per node
    --nodes-max     maximum sub-nodes per node
    --text-min      minimum text length
    --text-max      maximum text length
    --users-min     minimum users
    --users-max     maximum users

EOF
        exit(1);
      }

    readAlphabet($main::config{'alphabet'});

    $users = getRandom($main::config{'users-min'}, $main::config{'users-max'});
    $lmax = getRandom($main::config{'depth-min'}, $main::config{'depth-max'});

    xmlHeader($users);
    xmlNode($users, 1, $lmax);
    xmlFooter();
  }


# ##############################################################################
#
# Description     create a pseudo random number (of bad quality)
# Author          Harry Brueckner
# Date            2005-06-01
# Arguments       $min - minimum number to create
#                 $max - maximum number to create
# Return          integer random number
#
sub getRandom
  {
    my ($min, $max) = @_;

    return int(rand($max - $min) + $min + 1);
  }


# ##############################################################################
#
# Description     create a random string of the defined alphabet
# Author          Harry Brueckner
# Date            2005-06-01
# Arguments       void
# Return          string of the given alphabet
#
sub getString
  {
    my ($length, $maxalpha, $string);

    $maxalpha = $#main::alphabet;
    $length = getRandom($main::config{'text-min'}, $main::config{'text-max'});

    $string = '';
    for (my $i = 0; $i < $length; $i++)
      {
        $string .= $main::alphabet[getRandom(0, $maxalpha)];
      }

    return $string;
  }


# ##############################################################################
#
# Description     create a timestamp
# Author          Harry Brueckner
# Date            2005-06-01
# Arguments       void
# Return          timestamp format 'YYYY-MM-DD hh:mm:ss'
#
sub getTimestamp
  {
    return sprintf('%04d-%02d-%02d %02d:%02d:%02d',
        localtime -> year() + 1900,
        localtime -> mon() + 1,
        localtime -> mday(),
        localtime -> hour(),
        localtime -> min(),
        localtime -> sec());
  }


# ##############################################################################
#
# Description     read the alphabet file
# Author          Harry Brueckner
# Date            2005-06-01
# Arguments       $filename   - the filename to read
# Return          void; the alphabet is stored in @main::alphabet
#
sub readAlphabet
  {
    my ($filename) = @_;
    my ($line);

    if (!-f $filename)
      {
        die "file '" . $filename . "' does not exist.\n";
        exit(1);
      }

    open(FILE, $filename) ||
        die "can not read file '" . $filename . "'.\n";
    while (defined($line = <FILE>))
      {
        $line =~ s/[\r\n]$//g;
        push(@main::alphabet, $line);
      }
    close(FILE);
  }


# ##############################################################################
#
# Description     print the xml footer
# Author          Harry Brueckner
# Date            2005-06-01
# Arguments       void
# Return          void
#
sub xmlFooter
  {
    print <<EOF;
</root>
EOF
  }


# ##############################################################################
#
# Description     print the xml header
# Author          Harry Brueckner
# Date            2005-06-01
# Arguments       $users - maximum number of users
# Return          void
#
sub xmlHeader
  {
    my ($users) = @_;
    my ($tstamp) = getTimestamp();

    print <<EOF;
<?xml version="1.0" encoding="ISO-8859-1"?>
<root created-by="1" created-on="$tstamp" modified-by="1" modified-on="$tstamp">
  <template created-on="$tstamp"/>
  <editor created-on="$tstamp">
    <user uid="1" created-on="$tstamp">unknown</user>
EOF
    for (my $i = 2; $i < $users; $i++)
      {
        print sprintf('    <user uid="%d" created-on="%s">user %d</user>%s',
            $i, $tstamp, $i, "\n");
      }
    print sprintf('    <user uid="%d" created-on="%s">%s</user>%s',
        $users, $tstamp,
        'Console Password Manager (This is just a test key.) &lt;cpm@testdomain.org&gt;',
        "\n");
    print <<EOF;
  </editor>
EOF
  }


# ##############################################################################
#
# Description     print a xml node and all it's sub-nodes (recursively)
# Author          Harry Brueckner
# Date            2005-06-01
# Arguments       $users  - maximum number of possible users
#                 $lnow   - current level
#                 $lmax   - maximum possible level
# Return          void
#
sub xmlNode
  {
    my ($users, $lnow, $lmax) = @_;
    my ($nmax, $tstamp, $uid);

    $nmax = getRandom($main::config{'nodes-min'}, $main::config{'nodes-max'});
    $tstamp = getTimestamp();

    for (my $n = 0; $n < $nmax; $n++)
      {
        $uid = getRandom(1, $users);

        print " " x ($lnow * 2);
        print sprintf('<node label="%s" created-by="%d" created-on="%s">%s',
            getString(), $uid, $tstamp, "\n");

        if (getRandom(1, 100) < 50)
          {
            print " " x ($lnow * 2);
            print sprintf('  <comment created-by="%d" created-on="%s">%s</comment>%s',
                $uid,
                $tstamp,
                getString(),
                "\n");
          }

        if ($lnow < $lmax)
          { xmlNode($users, $lnow + 1, $lmax); }

        print " " x ($lnow * 2);
        print "</node>\n";
      }
  }


# ##############################################################################

main();


# ##############################################################################

