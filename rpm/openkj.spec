Name:           openkj
Version:        0.11.0 
Release:        1%{?dist}
Summary:        Karaoke show hosting software

License:        GPL
URL:            https://openkj.org
Source0:        

BuildRequires:  qt5-qtbase-devel qt5-qtsvg-devel qt5-qtmultimedia-devel gstreamer1-devel
Requires:       qt5-qtbase qt5-qtsvg qt5-qtmultimedia gstreamer1

%description
Karaoke hosting software targeted at professional KJ's.  Includes rotation management, break music player,
key changer, and all of the various bits and pieces required to host karaoke.

%prep
%autosetup


%build
qmake-qt5
%make_build


%install
rm -rf $RPM_BUILD_ROOT
%make_install


%files
%license add-license-file-here
%doc add-docs-here



%changelog
* Tue Aug 15 2017 T. Isaac Lightburn <isaac@hozed.net>
- 
