Name:           dhcpsnoopd
Version:        0.01
Release:        1%{?dist}
Summary:        dhcpsnoopd is a pluggable dhcpv6 snooping daemon

License:        Apache-2.0 GPL-2.0
URL:            https://github.com/Natolumin/dhcpsnoopd
Source0:        https://github.com/Natolumin/dhcpsnoopd/archive/%{name}-%{version}.tar.gz

BuildRequires:  kea-devel >= 1.2.0
BuildRequires:  libmnl-devel
BuildRequires:  systemd

%description
dhcpsnoopd is a DHCPv6 snooping daemon, which calls whatever module you wish to define in a callback library

%prep
%setup -q


%build
%cmake .
%make_build


%install
%make_install


%files
%doc README.md

%attr(550, root, root) %caps(CAP_NET_ADMIN=ep) %{_bindir}/%{name}

%{_libdir}/%{name}/modules/*.so

%license COPYING


%changelog
* Sat Aug 12 2017 Anatole Denis <natolumin@rezel.net>
- First packaging
