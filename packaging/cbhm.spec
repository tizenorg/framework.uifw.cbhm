#sbs-git:slp/pkgs/c/cbhm cbhm 0.1.0 a67e97190313d19025925d8b9fd0aa9da3d0dc6a
Name:       cbhm
Summary:    cbhm application
Version:    0.1.108
Release:    1
Group:      TO_BE/FILLED_IN
License:    Flora Software License
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/cbhm.manifest 
Source1:    cbhm.service
BuildRequires:  cmake
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(appcore-common)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(xrandr)
BuildRequires:  pkgconfig(xcomposite)
BuildRequires:  pkgconfig(xi)
BuildRequires:  pkgconfig(svi)
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  edje-tools

%description
Description: cbhm application


%prep
%setup -q


%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}
cp %{SOURCE1001} .
make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_libdir}/systemd/user/tizen-mobile.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/user/cbhm.service
ln -s ../cbhm.service %{buildroot}%{_libdir}/systemd/user/tizen-mobile.target.wants/cbhm.service

mkdir -p %{buildroot}/etc/rc.d/rc3.d
ln -sf ../../init.d/cbhm %{buildroot}/etc/rc.d/rc3.d/S95cbhm


%files
%manifest cbhm.manifest
%defattr(-,root,root,-)
%{_sysconfdir}/init.d/cbhm
%{_sysconfdir}/rc.d/rc3.d/S95cbhm
%{_bindir}/cbhm
%{_datadir}/cbhm/icons/cbhm_default_img.png
%{_datadir}/edje/cbhmdrawer.edj
%{_libdir}/systemd/user/cbhm.service
%{_libdir}/systemd/user/tizen-mobile.target.wants/cbhm.service

