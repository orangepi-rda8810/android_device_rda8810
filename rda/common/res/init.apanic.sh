#!/system/bin/sh
#Collect apanic data, free resources and re-arm trigger

if [ -f "/proc/apanic_console" ]
then
cat /proc/apanic_console > /data/dontpanic/apanic_console
chown root:log /data/dontpanic/apanic_console
chmod 0640 /data/dontpanic/apanic_console
fi

if [ -f "/proc/apanic_threads" ]
then
cat /proc/apanic_threads > /data/dontpanic/apanic_threads
chown root:log /data/dontpanic/apanic_threads
chmod 0640 /data/dontpanic/apanic_threads
fi

if [ -f "/proc/apanic_console" ]
then
echo 1 > /proc/apanic_console
fi
