#sbs-git:slp/pkgs/c/cbhm cbhm 0.1.0 a67e97190313d19025925d8b9fd0aa9da3d0dc6a
Name:       cbhm
Summary:    cbhm application
Version:    0.1.160r08
Release:    1
Group:      TO_BE/FILLED_IN
License:    APLv2
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(appcore-common)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(xrandr)
BuildRequires:  pkgconfig(xi)
BuildRequires:  pkgconfig(enotify)
BuildRequires:  edje-tools
BuildRequires:    pkgconfig(libsystemd-daemon)
%{?systemd_requires}

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
mkdir -p %{buildroot}/usr/share/license
cp %{_builddir}/%{buildsubdir}/LICENSE %{buildroot}/usr/share/license/%{name}

mkdir -p %{buildroot}/opt/var/.cbhm_files
mkdir -p %{buildroot}/usr/lib/systemd/user/core-efl.target.wants

mkdir -p %{buildroot}/etc/smack/accesses.d/
cp %{_builddir}/%{buildsubdir}/cbhm.rule %{buildroot}/etc/smack/accesses.d/cbhm.rule
ln -s ../cbhm.service  %{buildroot}/usr/lib/systemd/user/core-efl.target.wants/cbhm.service

%post
echo "INFO: System should be restarted or execute: systemctl --user daemon-reload from user session to finish service installation."
//RSA Only for folder access control
chown app:app /opt/var/.cbhm_files/

%preun

%postun

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_bindir}/cbhm
%{_datadir}/cbhm/icons/cbhm_default_img.png
%{_datadir}/edje/cbhmdrawer.edj
/usr/lib/systemd/user/cbhm.service
/usr/lib/systemd/user/core-efl.target.wants/cbhm.service
/usr/share/license/%{name}
/etc/smack/accesses.d/cbhm.rule
/opt/var/.cbhm_files
