Name:		ptyopen
Version:	0.95
Release:	1%{?dist}
Summary:	Runs programs with a pseudo-terminal
Group:		System Environment/System Tools
License:	GPLv3
URL:		https://github.com/F-i-f/%{name}
Source:		https://github.com/F-i-f/%{name}/releases/download/v%{version}/%{name}-%{version}.tar.gz
BuildRequires:	gcc

%description
Ptyopen allows to run programs like if they were connected to a physical terminal.
This is handy for scripting programs that require a terminal

%prep
%setup -q

%build
%configure --enable-compiler-warnings --docdir="%{_docdir}/%{name}"
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
%make_install

%files
%{_bindir}/ptyopen
%{_mandir}/man1/ptyopen.1.gz
%{_docdir}/%{name}

%changelog
* Tue Sep 10 2024 Philippe Troin <phil@fifi.org> - 0.95-1
- Upstream updated to 0.95.
- Package extra doc files.

* Tue Apr 23 2024 Philippe Troin <phil@fifi.org> - 0.94-4
- Disable implicit int warnings (configure fails otherwise).

* Wed Oct 16 2019 Philippe Troin <phil@fifi.org> - 0.94-3
- Add BuildRequires: gcc.
- Lint spec.

* Tue Jun  5 2012 Philippe Troin <phil@fifi.org> - 0.94-2
- Add a group.

* Mon Jun  4 2012 Philippe Troin <phil@fifi.org> - 0.94-1
- Created.
