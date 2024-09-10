ptyopen
=======
runs a program on a fake terminal
=================================

## Documentation

**ptyopen** comes with an [extensive manual
page](https://htmlpreview.github.io/?https://raw.githubusercontent.com/F-i-f/ptyopen/master/ptyopen.1.html).

[View](https://htmlpreview.github.io/?https://raw.githubusercontent.com/F-i-f/ptyopen/master/ptyopen.1.html) or
download the manual page as:
[[HTML]](https://raw.githubusercontent.com/F-i-f/ptyopen/master/ptyopen.1.html),
[[PDF]](https://raw.githubusercontent.com/F-i-f/ptyopen/master/ptyopen.1.pdf) or
[[ROFF]](https://raw.githubusercontent.com/F-i-f/ptyopen/master/ptyopen.1).

## License

**ptyopen** is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see [http://www.gnu.org/licenses/].

## Building from source

### Requirements

* C Compiler (eg. gcc)

* Make (eg. GNU make)

* autotools (autoconf, automake) is only required if building from the
  repository.

* groff is optionally needed to generate the man page hard copies
  (HTML & PDF). It is only needed if you intend to update the manual
  page ROFF source.

### From a release

Download the [latest release from
GitHub](https://github.com/F-i-f/ptyopen/releases/download/v0.94/ptyopen-0.94.tar.gz)
or the [secondary mirror](http://ftp.fifi.org/phil/ptyopen/ptyopen-0.94.tar.gz):

* [Primary Site (GitHub)](https://github.com/F-i-f/ptyopen/releases/):

  * Source:
	[https://github.com/F-i-f/ptyopen/releases/download/v0.94/ptyopen-0.94.tar.gz](https://github.com/F-i-f/ptyopen/releases/download/v0.94/ptyopen-0.94.tar.gz)

  * Signature:
	[https://github.com/F-i-f/ptyopen/releases/download/v0.94/ptyopen-0.94.tar.gz.asc](https://github.com/F-i-f/ptyopen/releases/download/v0.94/ptyopen-0.94.tar.gz.asc)

* [Secondary Site](http://ftp.fifi.org/phil/ptyopen/):

  * Source:
	[http://ftp.fifi.org/phil/ptyopen/ptyopen-0.94.tar.gz](http://ftp.fifi.org/phil/ptyopen/ptyopen-0.94.tar.gz)

  * Signature:
	[http://ftp.fifi.org/phil/ptyopen/ptyopen-0.94.tar.gz.asc](http://ftp.fifi.org/phil/ptyopen/ptyopen-0.94.tar.gz.asc)


The source code release are signed with the GPG key ID `0x88D51582`,
available on your [nearest GPG server](https://pgp.mit.edu/) or
[here](http://ftp.fifi.org/phil/GPG-KEY).

You can also find all releases on the [GitHub release
page](https://github.com/F-i-f/ptyopen/releases/) and on the [secondary
mirror](http://ftp.fifi.org/phil/ptyopen/).

When downloading from the GitHub release pages, be careful to download
the source code from the link named with the full file name
(_ptyopen-0.94.tar.gz_), and **not** from the links marked _Source code
(zip)_ or _Source code (tar.gz)_ as these are repository snapshots
generated automatically by GitHub and require specialized tools to
build (see [Building from GitHub](#from-the-github-repository)).


After downloading the sources, unpack and build with:

```shell
tar xvzf ptyopen-0.94.tar.gz
cd ptyopen-0.94
./configure
make
make install
make install-pdf install-html # Optional
```

Alternately, you can create a RPM file by moving the source tar file
and the included `ptyopen.spec` in your rpm build directory and running:

```shell
rpmbuild -ba SPECS/ptyopen.spec
```

### From the GitHub repository

Clone the [repository](https://github.com/F-i-f/ptyopen.git):

```shell
git clone https://github.com/F-i-f/ptyopen.git
cd ptyopen
autoreconf -i
./configure
make
make install
make install-pdf install-html # Optional
```

## Changelog

See release notes in
[NEWS](https://github.com/F-i-f/ptyopen/blob/master/ChangeLog) or the
detailed [ChangeLog](https://github.com/F-i-f/ptyopen/blob/master/ChangeLog).

## Credits

**ptyopen** was written by Philippe Troin ([F-i-f on GitHub](https://github.com/F-i-f)).
