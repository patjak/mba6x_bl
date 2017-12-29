# Installation

You need to have the kernel headers (e.g. linux-header-generic) installed for the kernel you're compiling for. You also need the build tools (build-essential or similar).

* git clone git://github.com/patjak/mba6x_bl
* cd mba6x_bl
* make
* sudo make install
* sudo depmod -a

Note: You no longer need to specify mba6x_bl as your backlight device in /etc/X11/xorg.conf. This is now handled automatically by promoting the driver above the ACPI backlight interface.

At this point you can load the module

    sudo modprobe mba6x_bl

Then restart X (usually log in / log out will do the trick). The module should automatically load on next boot on any MacBook Air 6,x.

### If brightness control still doesn't work:

You may need to follow the directions suggested in [#65](https://github.com/patjak/mba6x_bl/issues/65) and do the following:
1. Create `usr/share/X11/xorg.conf.d/98-mba6bl.conf`.
2. In that file, enter the following content (from  [here](https://github.com/patjak/mba6x_bl/blob/master/98-mba_bl.conf)):
```
Section "Device"
  Identifier "Intel Graphics"
  Driver "Intel"
  Option "Backlight"  "mba6x_backlight"
EndSection
```
3. Remove `/usr/share/X11/xorg.conf.d/20-intel.conf` or move it to a backup directory.
4. Log in/log out or `$ reboot`.


## Debian dkms package

You can either follow the long route <https://wiki.kubuntu.org/Kernel/Dev/DKMSPackaging>, or
download a pre-made .deb.zip file of v1.1.0 from
<https://github.com/patjak/mba6x_bl/files/505288/mba6xbl-dkms_1.1.0_all.deb.zip>,
which can be installed with `dpkg -i`. (Installing v1.0.0 can result in build errors; see [#55](https://github.com/patjak/mba6x_bl/issues/55).)

## Ubuntu specific instructions
<https://help.ubuntu.com/community/MacBookAir6-2/Trusty#Backlight>

## Arch Linux dkms package
An Arch Linux prepared package can be found at
<https://aur.archlinux.org/packages/mba6x_bl-dkms/>

## Gentoo ebuild
A Gentoo ebuild can be had at <https://github.com/chrisperelstein/gentoo-overlay/blob/master/x11-drivers/mba6x_bl/mba6x_bl-9999.ebuild>. You can install with `layman -o https://github.com/chrisperelstein/gentoo-overlay/raw/master/overlay.xml -a chrisperelstein`, unmasking by adding `=x11-drivers/mba6x_bl-9999::chrisperelstein **` to /etc/portage/package.accept_keywords, followed by `emerge mba6x_bl`.
