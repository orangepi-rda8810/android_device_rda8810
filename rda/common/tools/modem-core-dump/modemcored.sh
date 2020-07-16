#!/system/bin/sh

# Copyright (C) 2015 RDA Microsystems
#
# Modem crash save daemon
# Wait for a crash event and save the corefiles and other important information
#
# TODO:
# - Check SD Card which have space
# - Rotate logs / delete old logs
#
# Debug:
# 0x9db09db1 > /sys/devices/platform/rda-comreg0.0/magic
# 0x9db09db0 > /sys/devices/platform/rda-comreg0.0/magic

# Configuration
poll_interval=10000   # poll every 10 sec when the AP is running.
reboot_after_crash=no # change to 'yes' to make device reboot after crash

function echoi {  log -p i -t modemcored $@; }
function echov {  log -p v -t modemcored $@; }
function echoe {  log -p e -t modemcored $@; }

mess_dump_name="crash.txt"
wcore_dump_name="wcpucore.elf"
xcore_dump_name="xcpucore.elf"
logcat_dump_name="logcat.txt"

crash_listner=rda_crash_listner
sysfs_modem_crash_event="/sys/devices/platform/rda-comreg0.0/modem_crash"

# Path to SD cards. Prefer 'core_path_main'
core_sdcard="/storage/sdcard0"        # default
#core_sdcard_main="/storage/sdcard1"   # prefer


# Start daemon if the crash_listner exist.
if [[ -f $(which ${crash_listner}) ]]; then

	echoi "Starting: RDA modemcrashd"
	while [[ $(getprop app.emu.storage) == "" ]]; do
		sleep 10
	done

#	prefered_sdcard_exist=$(mount | grep ${core_sdcard_main})
#
#	if [[ -d ${core_sdcard_main} && "${prefered_sdcard_exist}" ]]; then
#		core_sdcard=${core_sdcard_main}
#	fi

	# Make sure "corefiles" directory exist.
	core_path=${core_sdcard}/corefiles
	mkdir -p ${core_path}

	# Change to corefile directory
	echov "logging to ${core_path}"
	cd ${core_path}

	while [ true ]; do

		crashed_cpu=$(${crash_listner} -t ${poll_interval})

		# Clear the crash indication
		echo 0 > ${sysfs_modem_crash_event}

		now=$(date +%Y%m%d_%H%M%S) # YYYYMMDD_HHMMSS (20151224_174742)

		case ${crashed_cpu} in
		xcpu | wcpu | xcpuwcpu)
			echov "${crashed_cpu} crashed"
			echov "Writing crash information to: ${core_path}/${now}..."

			# Create a log file for this crash
			crashlog=${now}${mess_dump_name}
			echo "${crashed_cpu} crashed!" > ${crashlog}
			echo "Linux version:" >> ${crashlog}
			cat /proc/version >> ${crashlog}
			echo "Linux cmdline:" >> ${crashlog}
			cat /proc/cmdline >> ${crashlog}
			echo "Modem version:" >> ${crashlog}
			cat /proc/modem_version >> ${crashlog}
			echo "Modem RFCalib:" >> ${crashlog}
			cat /proc/modem_rfcalib >> ${crashlog}

			# Dump contents of logcat to a file.
			logcat -d -f ${now}${logcat_dump_name}

			# Copy the corefiles to SD card.
			cp /proc/xcpucore ${now}${xcore_dump_name}
			cp /proc/wcpucore ${now}${wcore_dump_name}

			echov "Done writing crash information"

			# Wait a few seconds and reboot
			if [[ ${reboot_after_crash} == "yes" ]]; then
				sync
				sleep 5
				reboot
			fi
			;;
		*)
			echoi "Unknown CPU crashed(?)"
			;;
		esac
	done
else
	echoe "Crash listner, ${crash_listner}, not found!";
fi
