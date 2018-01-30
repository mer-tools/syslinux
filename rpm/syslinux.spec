# -*- rpm -*-
Summary: Kernel loader which uses a FAT, ext2/3 or iso9660 filesystem or a PXE network
Name: syslinux
Version: 4.06
Release: 1
License: GPLv2
Url: http://syslinux.zytor.com/
Group: System/Boot
Source0: %{name}-%{version}.tar.gz
ExclusiveArch: %{ix86} x86_64
BuildRequires: nasm >= 0.98.39
BuildRequires: perl
BuildRequires: python
BuildRequires: libuuid-devel

Source101: syslinux-rpmlintrc
Patch0:	0001-Remove-development-makefile-options.patch

Autoreq: 0
%ifarch x86_64
Requires: mtools
%define my_cc gcc
%else
Requires: mtools, libc.so.6
%define my_cc gcc -m32
%endif

# NOTE: extlinux belongs in /sbin, not in /usr/sbin, since it is typically
# a system bootloader, and may be necessary for system recovery.
%define _sbindir /sbin

%package devel
Summary: Development environment for SYSLINUX add-on modules
Group: Development/Libraries
Requires: syslinux

%description
SYSLINUX is a suite of bootloaders, currently supporting DOS FAT
filesystems, Linux ext2/ext3 filesystems (EXTLINUX), PXE network boots
(PXELINUX), or ISO 9660 CD-ROMs (ISOLINUX).  It also includes a tool,
MEMDISK, which loads legacy operating systems from these media.

%description devel
The SYSLINUX boot loader contains an API, called COM32, for writing
sophisticated add-on modules.  This package contains the libraries
necessary to compile such modules.

%package extlinux
Summary: The EXTLINUX bootloader, for booting the local system
Group: System/Boot
Requires: syslinux

%description extlinux
The EXTLINUX bootloader, for booting the local system, as well as all
the SYSLINUX/PXELINUX modules in /boot.

%package tftpboot
Summary: SYSLINUX modules in /var/lib/tftpboot, available for network booting
Group: Applications/Internet
Requires: syslinux

%description tftpboot
All the SYSLINUX/PXELINUX modules directly available for network
booting in the /var/lib/tftpboot directory.

%prep
%setup -q -n %{name}-%{version}/%{name}

%patch0 -p1

%build
make CC='%{my_cc}'

%install
rm -rf %{buildroot}
make CC='%{my_cc}' install-all \
	INSTALLROOT=%{buildroot} BINDIR=%{_bindir} SBINDIR=%{_sbindir} \
	LIBDIR=%{_libdir} DATADIR=%{_datadir} \
	MANDIR=%{_mandir} INCDIR=%{_includedir} \
	TFTPBOOT=/var/lib/tftpboot EXTLINUXDIR=/boot/extlinux

mkdir -p %{buildroot}/etc
( cd %{buildroot}/etc && ln -s ../boot/extlinux/extlinux.conf . )

%files
%defattr(-,root,root)
%doc COPYING
%{_bindir}/*
%dir %{_datadir}/syslinux
%{_datadir}/syslinux/*.com
%{_datadir}/syslinux/*.c32
%{_datadir}/syslinux/*.bin
%{_datadir}/syslinux/*.0
%{_datadir}/syslinux/memdisk
%{_datadir}/syslinux/dosutil/*
%{_datadir}/syslinux/diag/*

%files devel
%defattr(-,root,root)
%doc NEWS doc/*
%doc sample
%doc %{_mandir}/man*/*
%{_datadir}/syslinux/com32

%files extlinux
%defattr(-,root,root)
%{_sbindir}/extlinux
/boot/extlinux
%config /etc/extlinux.conf

%files tftpboot
%defattr(-,root,root)
/var/lib/tftpboot

%post extlinux
# If we have a /boot/extlinux.conf file, assume extlinux is our bootloader
# and update it.
if [ -f /boot/extlinux/extlinux.conf ]; then \
    extlinux --update /boot/extlinux ; \
elif [ -f /boot/extlinux.conf ]; then \
    mkdir -p /boot/extlinux && \
    mv /boot/extlinux.conf /boot/extlinux/extlinux.conf && \
    extlinux --update /boot/extlinux ; \
fi
