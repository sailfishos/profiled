Name:       profiled
Summary:    Profile daemon, manages user settings
Version:    1.0.11
Release:    1
License:    BSD
URL:        https://git.sailfishos.org/mer-core/profiled
Source0:    %{name}-%{version}.tar.bz2
Requires:   profiled-settings
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  doxygen

%description
Gets default set of settings from profile data packages
that are installed on the device .

Currently active profile and values within profile can
be changed using C language API implemented in libprofile.

Changes to values are broadcast as D-Bus signals so that
active clients can be avare of changes without polling.

%package doc
Summary:    API documentation for libprofile
Requires:   %{name} = %{version}-%{release}

%description doc
Doxygen generated files documenting the C language API used by
the client applications and the D-Bus API used by libprofile
for communication with profiled.

%package -n profileclient
Summary:    Command line test tool for profiled
Requires:   %{name} = %{version}-%{release}

%description -n profileclient
The profileclient tool can be used to query/change
profiles and profile values.

%package settings-default
Summary:    Default settings for profiled
Requires:   %{name} = %{version}-%{release}
Provides:   profiled-settings

%description settings-default
Default settings for profiled.

%package devel
Summary:    Development files for libprofile
Requires:   %{name} = %{version}-%{release}

%description devel
C language headers for building client applications using
profiled library.

%prep
%setup -q -n %{name}-%{version}

%define build_variables ROOT=%{buildroot} LIBDIR=%{_libdir} DLLDIR=%{_libdir} PKGCFGDIR=%{_libdir}/pkgconfig

%build
make %{build_variables} %{_smp_mflags}

%install
rm -rf %{buildroot}

make %{build_variables} install-profiled
make %{build_variables} install-libprofile
make %{build_variables} install-libprofile-dev
make %{build_variables} install-libprofile-doc
make %{build_variables} install-profileclient
rm %{buildroot}/%{_libdir}/libprofile.a

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%dir %{_sysconfdir}/profiled
%{_bindir}/%{name}
%{_libdir}/libprofile.so.*
%{_datadir}/dbus-1/services/com.nokia.profiled.service
%{_userunitdir}/profiled.service

%files doc
%defattr(-,root,root,-)
%{_docdir}/libprofile-doc/html/*
%{_mandir}/man3/*

%files -n profileclient
%defattr(-,root,root,-)
%{_bindir}/profileclient

%files settings-default
%defattr(-,root,root,-)
%{_sysconfdir}/profiled/10.meego_default.ini

%files devel
%defattr(-,root,root,-)
%{_includedir}/%{name}/*
%{_libdir}/libprofile.so
%{_libdir}/pkgconfig/profile.pc
