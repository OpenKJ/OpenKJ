Name:           openkj
Version:	1.3.48
Release:        1%{?dist}
Summary:        Karaoke show hosting software

License:        GPL
URL:            https://openkj.org
Source0:	openkj-1.3.48.tar.bz2

BuildRequires:  qt5-qtbase-devel qt5-qtsvg-devel qt5-qtmultimedia-devel gstreamer1-devel gstreamer1-plugins-base-devel
Requires:       qt5-qtbase qt5-qtsvg qt5-qtmultimedia gstreamer1 gstreamer1-plugins-good gstreamer1-plugins-bad-free unzip

%description
Karaoke hosting software targeted at professional KJ's.  Includes rotation management, break music player,
key changer, and all of the various bits and pieces required to host karaoke.

%prep
%setup -q


%build
cd OpenKJ
%{qmake_qt5} PREFIX=$RPM_BUILD_ROOT/usr
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
