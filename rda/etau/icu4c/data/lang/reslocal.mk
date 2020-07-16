LANG_CLDR_VERSION = 23

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

# Aliases without a corresponding xx.xml file (see icu-config.xml & build.xml)
LANG_SYNTHETIC_ALIAS = in.txt in_ID.txt zh_CN.txt\
 zh_HK.txt zh_Hans_CN.txt zh_Hant_MO.txt zh_Hant_TW.txt zh_MO.txt\
 zh_SG.txt zh_TW.txt id_ID.txt en_NH.txt en_RH.txt en_VU.txt en_ZW.txt

# All aliases (to not be included under 'installed'), but not including root.
LANG_ALIAS_SOURCE = $(LANG_SYNTHETIC_ALIAS)


# Ordinary resources
LANG_SOURCE = en.txt en_GB.txt my.txt id.txt\
 zh.txt zh_Hans.txt zh_Hans_HK.txt\
 zh_Hans_MO.txt zh_Hans_SG.txt zh_Hant.txt zh_Hant_HK.txt
