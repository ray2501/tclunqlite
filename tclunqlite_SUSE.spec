%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}

Name:          tclunqlite
Summary:       Tcl interface for UnQLite
Version:       0.2.6
Release:       4
License:       BSD
Group:         Development/Libraries/Tcl
Source:        https://sites.google.com/site/ray2501/tclunqlite/tclunqlite_0.2.6.zip
URL:           https://sites.google.com/site/ray2501/tclunqlite 
Buildrequires: tcl >= 8.1
BuildRoot:     %{buildroot}

%description
This is the UnQLite extension for Tcl using the Tcl Extension Architecture (TEA).

UnQLite is a in-process software library which implements a self-contained, 
serverless, zero-configuration, transactional NoSQL database engine.
This extension provides an easy to use interface for accessing UnQLite 
database files from Tcl.

%prep
%setup -q -n %{name}

%build
CFLAGS="%optflags" ./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}
make 

%install
make DESTDIR=%{buildroot} pkglibdir=%{directory}/%{_lib}/tcl/%{name}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%{directory}/%{_lib}/tcl
%{directory}/share/man/mann
