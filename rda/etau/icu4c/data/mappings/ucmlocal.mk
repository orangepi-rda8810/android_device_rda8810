# Copyright 2008 Google Inc. All Rights Reserved.
# Author: mscherer@google.com (Markus Scherer)

# A list of additional .ucm files to build for Google.
# Note: noop-*.ucm are for Android only to prevent 2022 security attack.


# If you are thinking of modifying this file for "etau", READ THIS.
#
# Instead of changing this file in "external/icu4c/" [this file is ignore],
#
# you should consider changing the same name file in "device/rda/etau/icu4c/" 
#
# Then, you need choose lunch etau to copy the file which you modify to the "external/icu4c/"
#
# And make it.
#
# To git push the change,you need push the change of "device/rda/etau/icu4c/" instead of "external/icu4c/"
#
# Don't forget change the "device/rda/etau/icu4c/stubdata/icudt51l-all.dat" and ""device/rda/etau/icu4c/stubdata/icudt51l-default.dat"

UCM_SOURCE_LOCAL=gsm-03.38-2000.ucm \
   docomo-shift_jis-2012.ucm \
   jisx-208.ucm \
   kddi-jisx-208-2007.ucm \
   kddi-shift_jis-2012.ucm \
   softbank-jisx-208-2007.ucm \
   softbank-shift_jis-2012.ucm \
   noop-cns-11643.ucm \
   noop-gb2312_gl.ucm \
   noop-iso-ir-165.ucm

UCM_SOURCE = ibm-437_P100-1995.ucm\
ibm-850_P100-1995.ucm\
ibm-852_P100-1995.ucm\
ibm-858_P100-1997.ucm\
ibm-1252_P100-2000.ucm\
ibm-1276_P100-1995.ucm\
ibm-950_P110-1999.ucm\
ibm-964_P110-1999.ucm\
ibm-1375_P100-2007.ucm\
ibm-5471_P100-2006.ucm\
iso-8859_10-1998.ucm\
iso-8859_14-1998.ucm\
windows-936-2000.ucm\
windows-950-2000.ucm\
jisx-212.ucm\
iso-ir-165.ucm cns-11643-1992.ucm\
ibm-5478_P100-1995.ucm\
icu-internal-25546.ucm lmb-excp.ucm \
icu-internal-compound-d1.ucm icu-internal-compound-d2.ucm icu-internal-compound-d3.ucm icu-internal-compound-d4.ucm\
icu-internal-compound-d5.ucm icu-internal-compound-d6.ucm icu-internal-compound-d7.ucm \
icu-internal-compound-s1.ucm icu-internal-compound-s2.ucm icu-internal-compound-s3.ucm icu-internal-compound-t.ucm \
euc-jp-2007.ucm

