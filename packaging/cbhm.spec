Name:       cbhm
Summary:    cbhm application
Version:    0.1.235
Release:    1
Group:      Applications
License:    Apache-2.0
URL:        http://www.samsung.com/
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
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:  edje-tools
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  gettext
%{?systemd_requires}

%description
Description: cbhm application

%prep
%setup -q

%build
%if "%{?tizen_profile_name}" == "wearable"
    export TARGET=2.3-wearable
%else
 %if "%{?tizen_profile_name}" == "mobile"
    export TARGET=2.3-mobile
 %else
   %if "%{?tizen_profile_name}" == "tv"
    export TARGET=2.3-mobile
    %endif
 %endif
%endif

%define PREFIX /usr/apps/org.tizen.cbhm

cd $TARGET && rm -rf CMakeFiles CMackCache.txt && cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX}
make %{?jobs:-j%jobs}

%install
%if "%{?tizen_profile_name}" == "wearable"
    export TARGET=2.3-wearable
%else
 %if "%{?tizen_profile_name}" == "mobile"
    export TARGET=2.3-mobile
 %else
   %if "%{?tizen_profile_name}" == "tv"
    export TARGET=2.3-mobile
    %endif
 %endif
%endif

cd $TARGET && %make_install

## systemd
mkdir -p %{buildroot}/usr/lib/systemd/user/core-efl.target.wants
mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
ln -s ../cbhm.service  %{buildroot}/usr/lib/systemd/user/core-efl.target.wants/cbhm.service
ln -s ../cbhm.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/cbhm.service

mkdir -p %{buildroot}/%{_datadir}/license
cp %{_builddir}/%{buildsubdir}/LICENSE %{buildroot}/%{_datadir}/license/%{name}


%post
echo "INFO: System should be restarted or execute: systemctl --user daemon-reload from user session to finish service installation."

chown -R 5000:5000  %{PREFIX}/share

%files
%defattr(-,root,root,-)
%{PREFIX}/bin/*
%{PREFIX}/share/edje/cbhmdrawer.edj
%{PREFIX}/share/locale/*
## systemd
%{_libdir}/systemd/user/cbhm.service
%{_libdir}/systemd/user/core-efl.target.wants/cbhm.service
%{_libdir}/systemd/system/cbhm.service
%{_libdir}/systemd/system/multi-user.target.wants/cbhm.service
%{_datadir}/license/%{name}
%manifest %{name}.manifest
