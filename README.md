# Installation

You need to have the kernel headers (e.g. linux-header-generic) installed for the kernel you're compiling for. You also need the build tools (build-essential or similar).

* git clone git://github.com/patjak/mba6x_bl
* cd mba6x_bl
* make
* sudo make install
* sudo depmod -a

You also need to notify the Intel Xorg driver what backlight device to use. Here is a minimal /etc/X11/xorg.conf

    Section "Device"
        Identifier      "Intel Graphics"
        Driver          "intel"
        Option          "Backlight"     "mba6x_backlight"
    EndSection

## Debian dkms package

You can either follow the long route <https://wiki.kubuntu.org/Kernel/Dev/DKMSPackaging>, or
download a pre-made .deb file from <http://miek.nl/downloads/2013/mba6xbl-dkms_0.0.1_all.deb>,
which can be installed with `dpkg -i`.
