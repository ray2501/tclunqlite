%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}

Name:          tclunqlite
Summary:       Tcl interface for UnQLite
Version:       0.3.3
Release:       2
License:       BSD
Group:         Development/Libraries/Tcl
Source:        https://sites.google.com/site/ray2501/tclunqlite/tclunqlite_0.3.3.zip
URL:           https://sites.google.com/site/ray2501/tclunqlite 
BuildRequires: autoconf
BuildRequires: make
BuildRequires: tcl-devel >= 8.1
Requires:      tcl >= 8.1
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
./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}
make 

%install
make DESTDIR=%{buildroot} pkglibdir=%{tcl_archdir}/%{name}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%{tcl_archdir}
%{directory}/share/man/mann
