Summary: A SDR recevier using libsdr.

%define version 0.1.0

License: GPL-2.0+
Name: sdr-rx
Group: Applications/Communications
Prefix: /usr
Release: 1
Source: sdr-rx-%{version}.tar.gz
URL: https://github.com/hmatuschek/sdr-rx
Version: %{version}
Buildroot: /tmp/libsdrrpm
BuildRequires: gcc-c++, cmake, portaudio-devel, fftw3-devel, rtl-sdr-devel, libsdr-devel
Requires: portaudio, fftw3, rtl-sdr, libsdr
%if 0%{?suse_version}
BuildRequires: libqt5-qtbase-devel
Requires: libqt5-qtbase 
%endif
%if 0%{?fedora}
BuildRequires: qt5-qtbase-devel
Requires: qt5-qtbase 
%endif

%description
A simple SDR receiver using libsdr.

%prep
%setup -q

%build
cmake -DCMAKE_BUILD_TYPE=RELEASE \
      -DCMAKE_INSTALL_PREFIX=$RPM_BUILD_ROOT/usr
make

%install
make install

%clean
rm -rf $RPM_BUILD_ROOT

 
%files
%defattr(-, root, root, -)
%attr(755, root, root) %{_prefix}/bin/sdr-rx
%{_prefix}/share/applications/sdr-rx.desktop
%{_prefix}/share/icons/sdr-rx.svg

