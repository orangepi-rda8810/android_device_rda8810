#!/system/bin/sh

#time=`date`
#echo start tar shell at $time >> /data/test.txt

tar zxvf /vendor/lib/gms.tar.gz -C /data/data/com.google.android.gms/

#timestamp=`getFileLastModifyTime /vendor/app/GmsCore.apk`

versionFile="/data/data/com.google.android.gms/shared_prefs/multidex.version.xml"
sourceFile="/vendor/app/GmsCore.apk"

#rm $versionFile
#echo "<?xml version='1.0' encoding='utf-8' standalone='yes' ?>" >> $versionFile
#echo "<map>" >> $versionFile
#echo "    <long name=\"timestamp\" value=\"${timestamp}000\" />" >> $versionFile
#echo "    <long name=\"crc\" value=\"1188498414\" />" >> $versionFile
#echo "    <int name=\"dex.number\" value=\"3\" />" >> $versionFile
#echo "</map>" >> $versionFile

modifyVersionXml $sourceFile $versionFile

chown 10000:10000 $versionFile

setprop multidex.finish.flag true

#time=`date`
#echo timestamp=$timestamp >> /data/test.txt
#echo end tar shell at $time >>/data/test.txt
