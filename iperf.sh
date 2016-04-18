#!/bin/bash -ev
which fping 2> /dev/null || ( echo "need to install fping" ; exit )
which iperf3 2> /dev/null || ( echo "need to install iperf3" ; exit )
ip link add name veth-host type veth peer name veth-guest
ip link set dev veth-host up
ip link set dev veth-guest up
#ip link set dev veth-host mtu 9000
#ip link set dev veth-guest mtu 9000
ip addr add 172.18.0.2/28 dev veth-host
ip netns add guest
ip link set dev veth-guest netns guest
ip netns exec guest ip li set dev veth-guest up
ip netns exec guest ip addr add 172.18.0.1/28 dev veth-guest
ip netns exec guest fping 172.18.0.2
ip netns exec guest iperf3 -s &
read -t 3 || :
iperf3 -c 172.18.0.1
# iperf3 -c 172.18.0.1 -Z
# iperf3 -c 172.18.0.1 -Z -l 1M
# ip link del dev veth-host ; ip netns del guest ; killall iperf3
