#!/usr/bin/perl -w

# ##############################################################################
# this script gets our own CSV format and creates the XML body structure for CPM
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
sub displayXML
  {
    my ($xml) = @_;

    # host
    foreach my $host (sort keys %{$xml -> {'host'}})
      {
        print sprintf('  <node label="%s">%s',
            xmlEncode($host), "\n");
        if ($xml -> {'host'} -> {$host} -> {'comment'} ne '')
          {
            print sprintf('    <comment>%s</comment>%s',
                xmlEncode($xml -> {'host'} -> {$host} -> {'comment'}),
                "\n");
          }

        # service
        foreach my $service (sort keys %{$xml -> {'host'} -> {$host} ->
            {'service'}})
          {
            print sprintf('    <node label="%s">%s',
                xmlEncode($service), "\n");
            if ($xml -> {'host'} -> {$host} -> {'service'} -> {$service} ->
                {'comment'} ne '')
              {
                print sprintf('      <comment>%s</comment>%s',
                    xmlEncode($xml -> {'host'} -> {$host} -> {'service'} ->
                    {$service} -> {'comment'}),
                    "\n");
              }

            # user
            foreach my $user (sort keys %{$xml -> {'host'} -> {$host} ->
                {'service'} -> {$service} -> {'user'}})
              {
                print sprintf('      <node label="%s">%s',
                    xmlEncode($user), "\n");
                if ($xml -> {'host'} -> {$host} -> {'service'} -> {$service} ->
                    {'user'} -> {$user} -> {'comment'} ne '')
                  {
                    print sprintf('        <comment>%s</comment>%s',
                        xmlEncode($xml -> {'host'} -> {$host} -> {'service'} ->
                        {$service} -> {'user'} -> {$user} -> {'comment'}),
                        "\n");
                  }

                # password
                foreach my $password (sort keys %{$xml -> {'host'} -> {$host} ->
                    {'service'} -> {$service} -> {'user'} -> {$user} ->
                    {'password'}})
                  {
                    print sprintf('        <node label="%s">%s',
                        xmlEncode($password), "\n");
                    if ($xml -> {'host'} -> {$host} -> {'service'} ->
                        {$service} -> {'user'} -> {$user} -> {'password'} ->
                        {$password} -> {'comment'} ne '')
                      {
                        print sprintf('          <comment>%s</comment>%s',
                            xmlEncode($xml -> {'host'} -> {$host} ->
                            {'service'} -> {$service} -> {'user'} -> {$user} ->
                            {'password'} -> {$password} -> {'comment'}),
                            "\n");
                      }
                    print "        </node>\n";
                  }
                # /password

                print "      </node>\n";
              }
            # /user

            print "    </node>\n";
          }
        # /service

        print "  </node>\n";
      }
    # /host
  }


# ##############################################################################
sub main
  {
    my ($line, %xml);

    while (defined($line = <STDIN>))
      { processLine($line); }

    displayXML($main::xml);
  }


# ##############################################################################
sub processLine
  {
    my ($line) = @_;
    my (%data, @tmp);

    $line =~ s/[\r\n]//g;
    if ($line eq '')
      { return; }

    @tmp = split(/\t/, $line);

    if ($#tmp != 7)
      {
        print STDERR sprintf("line has invalid number of elements (%d)\n",
            $#tmp + 1);
        return;
      }

    $data{'host'}       = $tmp[0];
    $data{'comment 1'}  = $tmp[1];
    $data{'service'}    = $tmp[2];
    $data{'comment 2'}  = $tmp[3];
    $data{'user'}       = $tmp[4];
    $data{'comment 3'}  = $tmp[5];
    $data{'password'}   = $tmp[6];
    $data{'comment 4'}  = $tmp[7];

    # we remove the terminating ';'
    $data{'comment 4'} =~ s/;$//;

    $main::xml ->
        {'host'}      -> {$data{'host'}} ->
        {'comment'}   =   $data{'comment 1'};
    $main::xml ->
        {'host'}      -> {$data{'host'}} ->
        {'service'}   -> {$data{'service'}} ->
        {'comment'}   =   $data{'comment 2'};
    $main::xml ->
        {'host'}      -> {$data{'host'}} ->
        {'service'}   -> {$data{'service'}} ->
        {'user'}      -> {$data{'user'}} ->
        {'comment'}   =   $data{'comment 3'};
    $main::xml ->
        {'host'}      -> {$data{'host'}} ->
        {'service'}   -> {$data{'service'}} ->
        {'user'}      -> {$data{'user'}} ->
        {'password'}  -> {$data{'password'}} ->
        {'password'}  =   $data{'password'};
    $main::xml ->
        {'host'}      -> {$data{'host'}} ->
        {'service'}   -> {$data{'service'}} ->
        {'user'}      -> {$data{'user'}} ->
        {'password'}  -> {$data{'password'}} ->
        {'comment'}   =   $data{'comment 4'};
  }


# ##############################################################################
sub xmlEncode
  {
    my ($string) = @_;

    $string =~ s/&/&amp;/g;
    $string =~ s/</&lt;/g;
    $string =~ s/>/&gt;/g;

    return $string;
  }


# ##############################################################################

main();


# ##############################################################################

