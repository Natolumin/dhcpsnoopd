# Pluggable DHCPv6 snooping for linux

This is a dhcp snooping daemon for linux, which makes use of nflog

You need to intercept DHCPv6 traffic and redirect it to nflog in your firewall, for example :
```
nft add rule inet filter meta nfproto ipv6 udp sport 547 log group 8
```

Then, you run dhcpsnoopd with the nflog group number and the path to a hook library:
dhcpsnoopd 2 routehook.so

# Requirements

 * Kernel > 3.8 (There were some changes to nfnetlink and I don't have one anymore to try to support that)
 * Kernel >= 4.4 if you want to use the default rt hook, which uses the ipv6 expiry option for routes

## Libraries

(It might work with older versions, but I haven't tested it)

 * libmnl >= 1.0.4
 * libkea-dhcp++ >= 5.0.0 (kea 1.2)
 * Boost >= 1.44 (for Kea)

## Provided hooks

 * print_hook: Simply prints the packet out using the standard kea representation
 * route_hook: Parses prefix delegation responses and adds routes in the kernel to allow routing to the prefixes that 
   are delegated
 * duid_hook: Records a mapping between mac address and DUID, for example to use in reservations. When using dhcp with a 
   relay, this should be more reliable than the builtin detections in kea when privacy link-level addresses are used.
