#!/system/bin/sh
echo -e "PID\tSwap\tVmHWM\tVmRSS\tVmData\tVmStk\tVmExe\tVmLib\tVmPTE\tProc_Name"

for pid in `ls /proc | grep -v [^0-9]`
do
        if [ -f /proc/$pid/status ];then
        cat /proc/$pid/status > /status
        if [ $? -ne 0 ];then continue;fi
        swap=`grep Swap /status | awk '{print $2}'`
        if [ $? -ne 0 ];then continue;fi
        if [ $swap ];then
        vmhwn=`grep VmHWM /status | awk '{print $2}'`
        vmrss=`grep VmRSS /status | awk '{print $2}'`
        vmdata=`grep VmData /status | awk '{print $2}'`
        vmstk=`grep VmStk /status | awk '{print $2}'`
        vmexe=`grep VmExe /status | awk '{print $2}'`
        vmlib=`grep VmLib /status | awk '{print $2}'`
        vmpte=`grep VmPTE /status | awk '{print $2}'`
        name=`grep Name /status | awk '{print $2}'`
        echo -e "$pid\t$swap\t$vmhwn\t$vmrss\t$vmdata\t$vmstk\t$vmexe\t$vmlib\t$vmpte\t$name"
        fi
        fi
done | sort -k2 -n -r

