COLLATION_CLDR_VERSION = 23

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
COLLATION_SYNTHETIC_ALIAS = in.txt in_ID.txt zh_CN.txt zh_HK.txt\
zh_MO.txt zh_SG.txt zh_TW.txt



# All aliases (to not be included under 'installed'), but not including root.
COLLATION_ALIAS_SOURCE = $(COLLATION_SYNTHETIC_ALIAS)


# Empty locales, used for validSubLocale fallback.
COLLATION_EMPTY_SOURCE = id.txt id_ID.txt zh_Hans.txt zh_Hans_CN.txt my_MM.txt\
 zh_Hans_SG.txt zh_Hant_HK.txt zh_Hant_MO.txt zh_Hant_TW.txt zu.txt\
 en_150.txt en_AG.txt en_AS.txt\
 en_AU.txt en_BB.txt en_BE.txt en_BM.txt en_BS.txt\
 en_BW.txt en_BZ.txt en_CA.txt en_CM.txt en_DM.txt\
 en_FJ.txt en_FM.txt en_GB.txt en_GD.txt en_GG.txt\
 en_GH.txt en_GI.txt en_GM.txt en_GU.txt en_HK.txt\
 en_IE.txt en_IM.txt en_IN.txt en_JE.txt en_JM.txt\
 en_KE.txt en_KI.txt en_KN.txt en_KY.txt en_LC.txt\
 en_LR.txt en_LS.txt en_MG.txt en_MH.txt en_MP.txt\
 en_MT.txt en_MU.txt en_MW.txt en_NA.txt en_NG.txt\
 en_NZ.txt en_PG.txt en_PH.txt en_PK.txt en_PR.txt\
 en_PW.txt en_SB.txt en_SC.txt en_SG.txt en_SL.txt\
 en_SS.txt en_TC.txt en_TO.txt en_TT.txt en_TZ.txt\
 en_UG.txt en_UM.txt en_VC.txt en_VG.txt en_VI.txt\
 en_VU.txt en_WS.txt en_ZA.txt en_ZM.txt en_ZW.txt\



# Ordinary resources
COLLATION_SOURCE = $(COLLATION_EMPTY_SOURCE) zh.txt my.txt zh_Hant.txt\
en.txt en_US.txt en_US_POSIX.txt

