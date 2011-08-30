Name:       cbhm
Summary:    cbhm application
Version:    0.1.0
Release:    1
Group:      TO_BE/FILLED_IN
License:    TO_BE/FILLED_IN
Source0:    cbhm-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  x11-xserver-utils-ex
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(appcore-common)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(mm-common)



%description
Description: cbhm application




%prep
%setup -q
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

%build
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc5.d/
ln -s %{_sysconfdir}/init.d/system_server.sh %{buildroot}%{_sysconfdir}/rc.d/rc5.d/S00system-server

%post 
ln -s /etc/init.d/cbhm /etc/rc.d/rc3.d/S95cbhm


%files 
%defattr(-,root,root,-)
%dir %{_sysconfdir}/init.d/cbhm
%{_bindir}/cbhm
%{_datadir}/cbhm/sounds/14_screen_capture.wav
%{_datadir}/cbhm/icons/cbhm_default_img.png
%{_datadir}/edje/cbhmdrawer.edj
