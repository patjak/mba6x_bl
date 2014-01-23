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

At this point you can load the module

    sudo modprobe mba6x_bl
    
Then restart X (usually log in / log out will do the trick). If things are working as expected, make the module load automatically at boot. See your distributions documentation for how to do this. I will add DMI detection for the module which will take care of this last step in the future.

## Debian dkms package

You can either follow the long route <https://wiki.kubuntu.org/Kernel/Dev/DKMSPackaging>, or
download a pre-made .deb file from <http://miek.nl/downloads/2013/mba6xbl-dkms_0.0.1_all.deb>,
which can be installed with `dpkg -i`.
