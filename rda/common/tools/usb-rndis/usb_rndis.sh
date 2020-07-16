#!/system/bin/sh
echo 'Step1: setprop rndis,adb' >&2
setprop sys.usb.config 'rndis,adb'
until [ "$(getprop sys.usb.state)" = 'rndis,adb' ] ; do sleep 1 ; done
echo 'Step2: config usb iface' >&2
ip rule add from all lookup main
ip addr flush dev rndis0
ip addr add 192.168.42.129/24 dev rndis0
ip link set rndis0 up
echo 1 > /proc/sys/net/ipv4/ip_forward
sleep 0.3
echo 'Step3: enable nat' >&2
iptables -t nat -I POSTROUTING -o ppp0 -j MASQUERADE
iptables -t nat -A natctrl_nat_POSTROUTING -o ppp0 -j MASQUERADE
iptables -A natctrl_FORWARD -i ppp0 -o rndis0 -m state --state ESTABLISHED,RELATED -g natctrl_tether_counters
iptables -A natctrl_FORWARD -i rndis0 -o ppp0 -m state --state INVALID -j DROP
iptables -A natctrl_FORWARD -i rndis0 -o ppp0 -g natctrl_tether_counters
iptables -D natctrl_FORWARD -j DROP
iptables -A natctrl_FORWARD -j DROP
sleep 0.3
echo 'Step4: start dnsmasq' >&2
dnsmasq --keep-in-foreground  --interface=rndis0 --no-poll --domain-needed  --dhcp-option-force=43,ANDROID_METERED --pid-file --dhcp-range=192.168.48.2,192.168.48.254,1h --dhcp-range=192.168.47.2,192.168.47.254,1h --dhcp-range=192.168.46.2,192.168.46.254,1h  --dhcp-range=192.168.45.2,192.168.45.254,1h --dhcp-range=192.168.44.2,192.168.44.254,1h  --dhcp-range=192.168.43.2,192.168.43.254,1h --dhcp-range=192.168.42.2,192.168.42.254,1h  --server=10.168.1.1 --listen-address=192.168.42.129
