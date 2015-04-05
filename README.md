Console Password Manager
========================

Description: This program is a ncurses based console tool to manage passwords
and store them public key encrypted in a file - even for more than one person.
The encryption is handled via GnuPG so the programs data can be accessed via
gpg as well, in case you want to have a look inside.
The data is stored as as zlib compressed XML so it's even possible to reuse the
data for some other purpose.

The software uses CDK (ncurses) to handle the user interface, libxml2 to store
the information, the zlib library to compress the data and the library GpgMe to
encrypt and decrypt the data securely.


Copyright
---------

Copyright (C) 2005-2009 Harry Brueckner

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or any later version.
This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.
You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

Contact: Harry Brueckner <harry_b@mm.st>
         Muenchener Strasse 12a
         85253 Kleinberghofen
         Germany


Features
--------

- data files can be encrypted for more than one person (public key encryption)
- data files are always signed by the last person who saved it so forging data files is not possible
- encryption is handled by the GPGME library so it's supposed to be very secure
- data inside the encryption is a gzipped XML file so almost nothing is known about the encrypted data
- the application memory is protected from paging
- no core dumps are created in case the program crashes
- the application is protected from ptrace attacks
- the runtime environment is carefully checked
- data files are en- and decryptable directly by gpg and gzip
- data is stored XML formatted
- backup files are created if possible
- data is validated using an internal DTD
- several passwords per account are possible to store
- it's possible to handle several data files, each encrypted for different people
- check of password strength and warnings about weak passwords (handled by cracklib)
- user definable hierarchy with unlimited depth
- long comments for any node in the hierarchy
- password generator
- there is only one password visible at a time
- searchable database from the command line
- user definable search patterns (e.g. user@hostname)
- several hits can be displayed at once (e.g. several accounts per host)
- conversion scripts for Password Management System (pms), Password Safe and CSV files


Platforms
---------

This software has been tested on the following platforms:
- Ubuntu Hardy
- Debian Woody, Sid and Sarge
- Gentoo Linux
- SuSE Linux

If you encounter memory problems, e.g. on Gentoo Linux, please read the section 'Memory issues' in the security section of this document.

In case you use CPM on another platform it would be nice if you could send me
a short email reporting success (or failure which I will fix as soon as
possible).


Download
--------

  * Web
    It can be downloard at [tar.gz](http://www.harry-b.de/downloads/cpm.tar.gz) or [tar.bz2](http://www.harry-b.de/downloads/cpm.tar.bz2)

  * Debian Linux
    The package is also available as a Debian package. The package is available at [here](http://debian.harry-b.de/binary/).

    To add this package to your APT-repository, just add the line
    ```bash
    deb http://debian.harry-b.de/ binary/
    ```
    to your sources list in /etc/apt/sources.list.

  * Gentoo Linux
    For Gentoo Linux a ebuild file is available at [here](http://debian.harry-b.de/gentoo/).

    First add PORTDIR_OVERLAY=/usr/local/portage to /etc/make.conf.
    To use this, you have to perform the following steps:
    ```bash
        $ mkdir -p /usr/local/portage/app-admin/cpm
        $ cp cpm-0.8_beta.ebuild /usr/local/portage/app-admin/cpm
        $ ebuild /usr/local/portage/app-admin/cpm/cpm-0.8_beta.ebuild digest
        $ emerge /usr/local/portage/app-admin/cpm/cpm-0.8_beta.ebuild
    ```
    where you have to replace the version number with the current version.

    The ebuild file was created by Marc Jauvin <marc@r4l.com>.
    Thanks alot for the support!


Installation requirements
-------------------------

To install this program, the following libraries are required:
- cdk (<= 4.9.10 or >= 5.0.20090215)
- crack
- dotconf
- gpgme
- ncurses
- xml2 (>= 2.6.16)

For CDK only versions up to 4.9.10 can be used. Higher versions have a bug
which can not handle empty widgets. I reported to the CDK developers on
2005-09-08 and it should be fixed in one of the upcoming version 5 releases.

On Debian systems, the package names are:
- cracklib-runtime
- libcdk5
- libcdk5-dev
- libcrack2
- libcrack2-dev
- libdotconf1.0
- libdotconf-dev
- libgpg-error-dev
- libgpgme11
- libgpgme11-dev
- libncurses5
- libncurses5-dev
- libxml2
- libxml2-dev
- txt2man
- zlib1g
- zlib1g-dev


Installation
------------

Installation should be quite simple if all requirements are met:
    1. ./configure
    2. make
    3. make check (this only works if it's compiled with -DTEST_OPTION)
    4. make install

In case the constant CRACKLIB_DICTPATH is not defined in your crack.h file, you
might have to tell configure where the dictionary files of libcrack are.  This
can be done by passing e.g.
'--with-crack-dict=/var/cache/cracklib/cracklib_dict' to configure.
Please note, that the file extension must not be specified.

If you don't have cracklib installed, you can turn off it's use by passing
--without-crack-lib to the configure command.

To handle memory problems, the option --without-memlock might help you. Please
read the section about memory issues in the security section very carefully.


Compiler settings
-----------------

For debugging, testing and configuration these labels can be defined:
    1. -DFORCE_CDK_V4 to force the correct calls for CDK version 4, even if
       it looks like version 5.
    2. -DKEY_DEBUG can be used to get information about the used keys during
       the signing process; it's use is for development only.
    3. -DMEMDEBUG can be used to find memory leaks and such nasty stuff.
       All memory operations show what they do and how much memory they
       allocate or free.
    4. -DMEMLOCK_LIMIT is used to define the memory limit to be defined for
       the max. locked memory check. See --with-memlock configure argument.
    5. -DNO_CRACKLIB can be used to not use the crack library - this reduces
       the security level of the application though (this gets automatically
       added if configure is started with --without-crack-lib).
    6. -DTEST_OPTION can be used to run some tests for the final program.
       It enables the command line option --testrun thus 'make check' can
       be used.
    7. -DTRACE_DEBUG enable can be used to enable the TRACE() function for
       debugging.


Configuration
-------------

The program tries to find it's configuration file at the following locations
in the given order:
    1. ${HOME}/.cpmrc
    2. /etc/cpm/cpmrc
    3. /etc/cpmrc

As soon as one of these files is found, it's used and the others (in case they
exist) are ignored.
You can find a default configuration in the file cpmrc-default.


Security issues
---------------

The binary should be suid root (mode 4755) to enable memory locking and
protection from ptrace attacks.

The applications runs a check on each startup on the following things:
  * if core dumps are disabled
  * if memory is locked from paging (so memory does not get written to swap
    space)
    WARNING: some computers (mostly notebooks) can create memory images for
             'hibernation'. It's not possible to protect the sensitive data
             from being written to those partitions!
  * if the application is protected from ptrace spying
  * if the application has environment checks enabled
  * if it's running without root privileges (right after program startup
    and memory locking, root privilges are dropped)

If one of these tests fail, a warning is displayed and a key must be
pressed to continue or abort the application.
The current security level can be displayed using the '--security' command line
argument.

    Memory issues
    -------------

    On some systems, the locked memory for an application is limited. This is
    the case e.g. on Gentoo and SuSE systems.
    In this case the limit is set to something like 32k which you can see using
    the command 'ulimit -l'. The interesting part of it's output is the line
    'max locked memory' - it should be at least 32M due to buffer bloat.
    I have no idea why this limit is used at all (except for some special
    purpose machines) and especially at this low limit. If anyone has an idea
    why it is used and set to such a small value please send me an email.

    cpm locks it's memory because it is the only way to prevent the memory from
    being swapped to disk, in case the operating system decides that it needs
    memory.
    If you want to disable memory locking (and take the risk that your
    passwords land in clear text on your harddisk) you can use the option
    --without-memlock to the configure command.
    !!! WARNING !!!
        It is NOT recommended to use this option - it opens a well known
        security leak!!!
    !!! WARNING !!!

    Unfortunately it is not possible to predict the exact amount of memory
    which is necessary to run cpm. It depends on the size of the XML structure
    and many other things which are not known at program startup.
    Hence, the default security procedure checks for at least 5120k of
    memory to lock. If you expect to handle alot of data with cpm, you can
    set this limit somewhat higher by using the --with-memlock option which
    specifies the amount of memory in kByte.

    Many thanks go to Daniel Schröder <mail@dschroeder.info> for helping me to
    track this problem down.


CLI usage
---------

For the command line the configuration tags TemplateName and SearchPattern are
used. The template names define the labels used for each level in the database.

Those labels are referred to by the SearchPattern definitions which then define
which database entries are combined to a string. This string then gets compared
to the search pattern passed on the command line.

On the command line interface it is possible to use regular expressions to
search through the database.


GUI usage
---------

To get a detailed help during runtime, press <CTRL><H>.


Data import
-----------

Right now there are three interfaces available for importing data. All these
are handled by the import.sh script which can be found in /usr/share/cpm.
The basic procedure always converts the 'foreign' format to cpm's own CSV
format and then this data gets imported.

The following import formats are supported:

    General interface
    -----------------
        The general interface imports a properly formatted CSV file.

    Password Safe
    -------------
        The passwordsafe interface can read CSV export files from Password Safe
        (by Bruce Schneier)

    PMS
    ---
        The PMS interface can read CSV files created by pms_export 


Structure of the XML file
-------------------------
```xml
<?xml version="1.0" encoding="ISO-8859-1"?>
<root version="0.2alpha">
  <template>
    <title level="1">host</title>
    <title level="2">service</title>
    <title level="3">user</title>
    <title level="4">password</title>
  </template>
  <editor>
    <user uid="1">unknown</user>
    <user uid="2">Harry Brueckner</user>
  </editor>
  <node label="label 1" >
    <node label="label 1.1">
      <comment>here goes the comment\nwith more than one line.</comment>
    </node>
    <node label="label 1.2" />
    <node label="label 1.3" />
  </node>
  <node label="label 2" >
    <node label="label 2.1" />
    <node label="label 2.2">
      <comment>here goes another comment</comment>
    </node>
    <node label="label 2.3" />
  </node>
</root>
```
