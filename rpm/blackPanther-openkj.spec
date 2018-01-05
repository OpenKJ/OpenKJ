%define debug_package %nil
%define git 0
%define oname OpenKJ

Name:           openkj
Version:        0.12
Release:        %mkrel 1
Summary:        Karaoke show hosting software
Summary(hu):    Karaoke műsort biztosító szoftver
License:        GPL
URL:            https://openkj.org
Vendor:		blackPanther Europe - www.blackpantheros.eu
Packager:	Charles K. Barcza <kbarcza@blackpanther.hu>
Source0:        %oname-%version.tar.gz
BuildRequires:  qtbase5-devel
BuildRequires:	devel(libQt5Svg)
BuildRequires:	devel(libQt5Multimedia)
BuildRequires:  devel(libgstreamer-1.0)
BuildRequires:  libgstreamer-plugins-base1.0-devel
Requires:       qtbase5-common libqt5svg5 libqt5multimedia5 gstreamer1.0-plugins-base

%description
Karaoke hosting software targeted at professional KJ's.
Includes rotation management, break music player, key changer, 
and all of the various bits and pieces required to host karaoke.

%prep
%if 0%git
%setup -q -n %oname
%else
%setup -q -n %oname-%version
%endif

%build
pushd %oname
 %oname
%qmake_qt5 PREFIX=%buildroot%_prefix
%make_build
popd

%install
pushd %oname
%make_install
popd

install -D -m 0644  %{buildroot}%{_datadir}/pixmaps/okjicon.svg %{buildroot}%{_iconsdir}/hicolor/scalable/apps/%{name}.svg

rm -rf %{buildroot}%{_datadir}/pixmaps
sed -i "s/^Icon=.*/Icon=%{name}/" %{buildroot}%{_datadir}/applications/%{name}.desktop
desktop-file-install --vendor="" \
  --add-category="Qt" \
  --add-category="X-blackPantherOS-Multimedia-Sound" \
  --add-category="X-blackPantherOS-CrossDesktop" \
  --remove-category="Application" \
  --dir %{buildroot}%{_datadir}/applications %{buildroot}%{_datadir}/applications/%{name}.desktop


%files
%doc *.md
%_bindir/%oname
%_datadir/applications/openkj.desktop
%_iconsdir/hicolor/scalable/apps/%name.svg

%changelog
* Fri Jan 05 2018 Charles Barcza <info@blackpanther.hu> 1.1-1bP
- build for blackPanther OS v16.x
--------------------------------------------------------------
* Fri Jan 05 2018 Charles Barcza <info@blackpanther.hu> 0.12-1bP
- build for blackPanther OS v16.x
--------------------------------------------------------------
* Fri Jan 05 2018 Charles Barcza <info@blackpanther.hu> 0.11-1bP
- build for blackPanther OS v16.x
--------------------------------------------------------------

