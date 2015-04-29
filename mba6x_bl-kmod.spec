# buildforkernels macro hint: when you build a new version or a new release
# that contains bugfixes or other improvements then you must disable the
# "buildforkernels newest" macro for just that build; immediately after
# queuing that build enable the macro again for subsequent builds; that way
# a new akmod package will only get build when a new one is actually needed
%global buildforkernels current

Name:       mba6x_bl
Version:    20150428.f1482dd
Release:    1%{?dist}
Summary:    Kernel module for MacBook Air backlight driver
Group:      System Environment/Kernel
License:    GPL
URL:        https://github.com/patjak/mba6x_bl
Source0:    https://raw.githubusercontent.com/patjak/mba6x_bl/master/mba6x_bl.c
Source1:    https://raw.githubusercontent.com/patjak/mba6x_bl/master/Makefile
Source2:    https://raw.githubusercontent.com/patjak/mba6x_bl/master/README.md
Source3:    https://raw.githubusercontent.com/patjak/mba6x_bl/master/LICENSE
Source4:    https://raw.githubusercontent.com/patjak/mba6x_bl/master/98-mba_bl.conf

BuildRequires:  %{_bindir}/kmodtool

ExclusiveArch:  i686 x86_64


%{!?kernels:BuildRequires: buildsys-build-rpmfusion-kerneldevpkgs-%{?buildforkernels:%{buildforkernels}}%{!?buildforkernels:current}-%{_target_cpu} }

# kmodtool does its magic here
%{expand:%(kmodtool --noakmod --target %{_target_cpu} --repo rpmfusion --kmodname %{name} --filterfile %{SOURCE0} %{?buildforkernels:--%{buildforkernels}} %{?kernels:--for-kernels "%{?kernels}"} 2>/dev/null) }

%description
MacBook Air backlight driver

%prep
rm -rf ${RPM_BUILD_ROOT}
# error out if there was something wrong with kmodtool
%{?kmodtool_check}

# print kmodtool output for debugging purposes:
kmodtool --target %{_target_cpu}  --repo rpmfusion --kmodname %{name} --filterfile %{SOURCE0} %{?buildforkernels:--%{buildforkernels}} %{?kernels:--for-kernels "%{?kernels}"} 2>/dev/null

for kernel_version in %{?kernel_versions} ; do
    mkdir -p _kmod_build_${kernel_version%%___*}
    cp %{SOURCE0} _kmod_build_${kernel_version%%___*}
    cp %{SOURCE1} _kmod_build_${kernel_version%%___*}
done

%build
for kernel_version in %{?kernel_versions}; do
    pushd _kmod_build_${kernel_version%%___*}
    make -C ${kernel_version##*___} M=`pwd` modules
    popd
done

%install
rm -rf ${RPM_BUILD_ROOT}

rm -rf ${RPM_BUILD_ROOT}
mkdir -p %{buildroot}/%{_docdir}/%{name}-%{version}
install -m 0644 %{SOURCE2} %{buildroot}/%{_docdir}/%{name}-%{version}
install -m 0644 %{SOURCE3} %{buildroot}/%{_docdir}/%{name}-%{version}

mkdir -p %{buildroot}/%{_sysconfdir}/X11/xorg.conf.d/
install -m 0644 %{SOURCE4} %{buildroot}/%{_sysconfdir}/X11/xorg.conf.d/

for kernel_version in %{?kernel_versions}; do
    pushd _kmod_build_${kernel_version%%___*}
    mkdir -p ${RPM_BUILD_ROOT}%{kmodinstdir_prefix}${kernel_version%%___*}%{kmodinstdir_postfix}
    install -m 0755 *.ko ${RPM_BUILD_ROOT}%{kmodinstdir_prefix}${kernel_version%%___*}%{kmodinstdir_postfix}
    popd
done

chmod 0755 $RPM_BUILD_ROOT%{kmodinstdir_prefix}*%{kmodinstdir_postfix}/* || :
%{?akmod_install}


# We need to define a userland package as described at
# http://rpmfusion.org/Packaging/KernelModules/Kmods2#userland_package

%package common
Summary:    Documentation and RPM provides for mba6x_bl driver
Provides:   %{name}-kmod-common = %{?epoch:%{epoch}:}%{version}

%description common
Documentation and RPM provides for mba6x_bl driver.

%files common
%defattr(-,root,root,-)
%doc %{_docdir}/%{name}-%{version}/*
%config(noreplace) %{_sysconfdir}/X11/xorg.conf.d/98-mba_bl.conf


%changelog
* Tue Apr 28 2015 Alexander Todorov <atodorov@redhat.com> - 20150428.f1482dd-1
- Initial build
