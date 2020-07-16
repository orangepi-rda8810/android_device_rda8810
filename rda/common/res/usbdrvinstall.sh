#!/system/bin/sh
mod_path="/vendor/modules"

insmod $mod_path/cdc-acm.ko
insmod $mod_path/usb-storage.ko
insmod $mod_path/usbhid.ko
insmod $mod_path/usbserial.ko
insmod $mod_path/usb_wwan.ko
insmod $mod_path/option.ko

