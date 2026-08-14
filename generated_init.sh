#!/usr/bin/env bash
set -euo pipefail

ip addr replace 192.168.50.1/24 dev enp1s0f0
ip link set enp1s0f0 up
ip link show enp1s0f0.100 >/dev/null 2>&1 || ip link add link enp1s0f0 name enp1s0f0.100 type vlan id 100 egress-qos-map 0:0 1:1 2:2 3:3 4:4 5:5 6:6 7:7
ip link set enp1s0f0.100 up
ip addr replace 192.168.150.1/24 dev enp1s0f0.100
