BRK_RES_CLDR_VERSION = 23

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
BRK_RES_SYNTHETIC_ALIAS =


# All aliases (to not be included under 'installed'), but not including root.
BRK_RES_ALIAS_SOURCE = $(BRK_RES_SYNTHETIC_ALIAS)


# List of dictionary files (dict).
BRK_DICT_SOURCE = cjdict.txt khmerdict.txt thaidict.txt


# List of break iterator files (brk).
BRK_SOURCE = char.txt line.txt sent.txt title.txt word.txt word_POSIX.txt


# Ordinary resources
BRK_RES_SOURCE = en.txt en_US.txt en_US_POSIX.txt

