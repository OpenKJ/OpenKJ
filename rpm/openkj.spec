Name:           openkj
Version:	1.7.137
Release:        1%{?dist}
Summary:        Karaoke show hosting software

License:        GPL
URL:            https://openkj.org
Source0:	openkj-1.7.137.tar.bz2

BuildRequires:  qt5-qtbase-devel qt5-qtsvg-devel qt5-qtmultimedia-devel gstreamer1-devel gstreamer1-plugins-base-devel taglib-devel taglib-extras-devel
Requires:       qt5-qtbase qt5-qtsvg qt5-qtmultimedia gstreamer1 gstreamer1-plugins-good gstreamer1-plugins-bad-free gstreamer1-plugins-ugly-free unzip gstreamer1-libav taglib taglib-extras

%description
Karaoke hosting software targeted at professional KJ's.  Includes rotation management, break music player,
key changer, and all of the various bits and pieces required to host karaoke.

%undefine _hardened_build
%define debug_package %{nil}

%prep
%setup -q


%build
cd OpenKJ
qmake-qt5 PREFIX=$RPM_BUILD_ROOT/usr

%make_build


%install
cd OpenKJ
%make_install

%files
/usr/bin/OpenKJ
/usr/share/applications/openkj.desktop
/usr/share/pixmaps/okjicon.svg

%changelog
* Tue Aug 15 2017 T. Isaac Lightburn <isaac@hozed.net>
- 
