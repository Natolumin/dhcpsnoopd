#!/bin/bash -e

ip netns add dhcptest
ip link add vethdhcp_h type veth peer name vethdhcp_ns
ip link set dev vethdhcp_ns netns dhcptest
ip link set up vethdhcp_h
ip netns exec dhcptest ip link set up vethdhcp_ns
# wait for dad to settle in the ns. Unfortunately net.ipv6.ip_nonlocal_bind doesn't let one listen on a nonsettled
# lladdr
sleep 2

cleanup() {
    ip netns del dhcptest
}

trap cleanup EXIT SIGTERM SIGINT

mkdir -p "${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
tmpdir=$(mktemp -d --tmpdir="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}" keaXXXXXX)
KEA_PIDFILE_DIR=$tmpdir KEA_LOCKFILE_DIR=$tmpdir /usr/sbin/kea-dhcp6 -c kea.json &

ip netns exec dhcptest dhclient -6 -w -d -P -N vethdhcp_ns
