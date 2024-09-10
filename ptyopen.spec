Name:		ptyopen
Version:	0.94
Release:	4%{?dist}
Summary:	Runs programs with a pseudo-terminal
Group:		System Environment/System Tools
License:	GPL
URL:		ftp://ftp.fifi.org/phil/%{name}
Source0:	%{URL}/%{name}-%{version}.tar.gz
BuildRequires:	gcc

%description
Ptyopen allows to run programs like if they were connected to a physical terminal.
This is handy for scripting programs that require a terminal

%prep
%setup -q


%build
export CFLAGS="$CFLAGS -Wno-implicit-int"
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%files
%{_bindir}/ptyopen
%{_mandir}/man1/ptyopen.1.gz
%{_docdir}/%{name}

%changelog
* Tue Apr 23 2024 Philippe Troin <phil@fifi.org> - 0.94-4
- Disable implicit int warnings (configure fails otherwise).

* Wed Oct 16 2019 Philippe Troin <phil@fifi.org> - 0.94-3
- Add BuildRequires: gcc.
- Lint spec.

* Tue Jun  5 2012 Philippe Troin <phil@fifi.org> - 0.94-2
- Add a group.

* Mon Jun  4 2012 Philippe Troin <phil@fifi.org> - 0.94-1
- Created.
