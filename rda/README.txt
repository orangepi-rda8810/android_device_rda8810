0. Terms
  - PLATFORM: remain unchanged for one chip
    TARGET_PLATFORM := rda8810
    ${ro.hardware}, stay the same with PLATFORM
    init.rda8810.rc, stay the same with PLATFORM
  - BOARD: change for different boards, rda8810base/aere, etc.
    TARGET_BOARD := rda8810base/aere/...

1. Differences among BOARDs
  - TARGET_BOARD: this is the name, rda8810base/aere/...
  - in different dir: same with the name
  - target: each board has its own target dir
  - $(TARGET_BOARD).mk: define for different board
  - device.mk: may vary for different board
  - customer.mk: may vary
  - system.prop: may vary
  - init.rda8810.rc: name remain the same, content may vary
  - recovery.fstab: may vary

2. BOARDS:
  - rda8810base
    + with 4Gb or larger nand, MLC nand with HEC
    + use yaffs for system
    + yaffs tag not in band
    + HVGA for Phone layout
  - aere
    + with 2Gb nand, SLC without HEC
    + use squashfs for system
    + other yaffs partition (userdata, customer) use yaffs tab in band
    + HVGA for Phone layout
  - bavi
    + with 4Gb nand, SLC without HEC
    + use yaffs for system
    + yaffs tag not in band
    + HVGA for Phone layout

3. Customer Drivers
  - general principle
    + in device/rda/driver/xxxx/, implement drivers for each peripherals
    + in customer.mk, choose which device driver to use
    + in each driver, they declare device by itsself, i.e. i2c bus id, i2c 
      address, gpio number, irq flags, etc
    + in target header files, i2c bus id, i2c address and gpio for each driver
      is defined, for purpose they may vary among different boards, even when
      they are same module
  - drivers in customer
    + touchscreen
    + camera sensor
    + gsensor
    + lsensor
    + psensor
  - drivers not in customer
    + LCD: this need to be in kernel, as it is needed to show logo

