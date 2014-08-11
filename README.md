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

## Debian dkms package

You can either follow the long route <https://wiki.kubuntu.org/Kernel/Dev/DKMSPackaging>, or
download a pre-made .deb file from <http://miek.nl/downloads/2014/mba6xbl-dkms_0.0.3_all.deb>,
which can be installed with `dpkg -i`.

## Ubuntu specific instructions
<https://help.ubuntu.com/community/MacBookAir6-2/Trusty#Backlight>

## Arch Linux dkms package
An Arch Linux prepared package can be found at
<https://aur.archlinux.org/packages/mba6x_bl-dkms/>
