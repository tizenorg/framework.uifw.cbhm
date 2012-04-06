#sbs-git:slp/pkgs/c/cbhm cbhm 0.1.0 a67e97190313d19025925d8b9fd0aa9da3d0dc6a
Name:       cbhm
Summary:    cbhm application
Version:    0.1.111
Release:    1
Group:      TO_BE/FILLED_IN
License:    Proprietary
Source0:    %{name}-%{version}.tar.gz
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
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}


%build
make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
%make_install


%preun
rm /etc/rc.d/rc3.d/S95cbhm
sync


%post
ln -s /etc/init.d/cbhm /etc/rc.d/rc3.d/S95cbhm
sync


%files
%defattr(-,root,root,-)
%{_sysconfdir}/init.d/cbhm
%{_bindir}/cbhm
%{_datadir}/cbhm/icons/cbhm_default_img.png
%{_datadir}/edje/cbhmdrawer.edj
