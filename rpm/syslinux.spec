# -*- rpm -*-
Summary: Kernel loader which uses a FAT, ext2/3 or iso9660 filesystem or a PXE network
Name: syslinux
Version: 6.04
Release: 1
License: GPLv2
Url: http://syslinux.zytor.com/
Source0: %{name}-%{version}.tar.gz
BuildRequires: nasm >= 0.98.39
BuildRequires: perl
BuildRequires: python
BuildRequires: libuuid-devel
Requires: mtools, libc.so.6

Source101: syslinux-rpmlintrc
Patch0001: 0001-Add-install-all-target-to-top-side-of-HAVE_FIRMWARE.patch
Patch0002: 0002-ext4-64bit-feature.patch
Patch0003: 0003-include-sysmacros-h.patch
Patch0004: 0004-Add-RPMOPTFLAGS-to-CFLAGS-for-some-stuff.patch
Patch0005: 0001-Fix-build-with-gcc8.patch
Patch0006: 0001-Patch-out-git-dependency.patch
Patch0007: 0016-strip-gnu-property.patch

Autoreq: 0

# NOTE: extlinux belongs in /sbin, not in /usr/sbin, since it is typically
# a system bootloader, and may be necessary for system recovery.
%define _sbindir /sbin

%package devel
Summary: Development environment for SYSLINUX add-on modules
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
Requires: syslinux
Requires(post): coreutils
Requires(post): util-linux

%description extlinux
The EXTLINUX bootloader, for booting the local system, as well as all
the SYSLINUX/PXELINUX modules in /boot.

%package tftpboot
Summary: SYSLINUX modules in /tftpboot, available for network booting
Requires: syslinux

%description tftpboot
All the SYSLINUX/PXELINUX modules directly available for network
booting in the /tftpboot directory.

%prep
%autosetup -p1 -n %{name}-%{version}/%{name}

%build
make clean
make bios

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_sbindir}
mkdir -p %{buildroot}%{_prefix}/lib/syslinux
mkdir -p %{buildroot}%{_includedir}

make bios install-all \
	INSTALLROOT=%{buildroot} BINDIR=%{_bindir} SBINDIR=%{_sbindir} \
	LIBDIR=%{_libdir} DATADIR=%{_datadir} \
	MANDIR=%{_mandir} INCDIR=%{_includedir} \
	TFTPBOOT=/tftpboot EXTLINUXDIR=/boot/extlinux \
	LDLINUX=ldlinux.c32

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
/tftpboot

%post extlinux
# If we have a /boot/extlinux.conf file, assume extlinux is our bootloader
# and update it.
if [ -f /boot/extlinux/extlinux.conf ]; then \
    MOUNTPOINT=$(stat "--format=%m" /boot/extlinux)
    DEVICE=$(findmnt --noheadings --output SOURCE $MOUNTPOINT)
    extlinux --update /boot/extlinux --device $DEVICE ; \
elif [ -f /boot/extlinux.conf ]; then \
    mkdir -p /boot/extlinux && \
    mv /boot/extlinux.conf /boot/extlinux/extlinux.conf && \
    extlinux --update /boot/extlinux ; \
fi
