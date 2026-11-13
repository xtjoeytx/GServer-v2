# PCID generation

On v4+ clients, the login packet contains a CSV formatted identity string.

The listserver keeps track of the identity strings.
If the first two fields are identical to a record in the database, and the second field is not empty, it returns the PCID assigned to that record.

If it fails to find a record, it checks a record that contains a match on fields 3 and 4.

If that still fails, it creates a new PCID.

---
### Platform

| Platform | Result |
|----------|--------|
| Windows  | win |
| MacOS | mac |
| Linux | linux |

---
## Windows

Fields:
1. platform
1. md5hash: product-id
1. md5hash: network-id
1. md5hash: disk-id
1. os-version

### Platform

win

### Product ID

> This field is often empty on 64-bit Windows due to SysWOW64 registry differences.

#### v4

MD5 of the binary DigitalProductId registry value (up to 512 bytes), read from `HKLM\Software\Microsoft\Windows\CurrentVersion` and, if that is empty, from `HKLM\Software\Microsoft\Windows NT\CurrentVersion`.

#### v5+

MD5 of the binary `DigitalProductId` registry value (up to 512 bytes), read from `HKLM\Software\Microsoft\Windows\CurrentVersion` and, if that is empty, from `HKLM\Software\Microsoft\Windows NT\CurrentVersion`.

If that returns empty, attempt the same for the `DigitalProductId4` registry value.

### Network ID

> This field is calculated on the first adapter that returns an address, so changing between ethernet and WiFi, or using a VPN or virtual-machine adapter, could change the result.  Windows can also give a random hardware address if the `MAC address randomization` feature is enabled for a WiFi card.

#### v4

MD5 of the 6-byte node of a `UuidCreateSequential` UUID (`UuidCreate` if the sequential call is missing). On Windows 2000 and later this node is a network card's MAC.

#### v5+

MD5 of the 6-byte hardware address of the first adapter in `GetAdaptersInfo` order that has an IPv4 address (an adapter whose address is all zeros is skipped). No UUID fallback on this path: with no such adapter the field is empty.

### Disk ID

> Can change on a complete Windows reformat.

MD5 of the 4-byte volume serial of `C:\` (little-endian, from `GetVolumeInformationA`).

### OS Version

#### v5+ only

`"<major>.<minor> <build> <service pack>"` from `GetVersionExA`; not hashed.

---
## MacOS

Fields:
1. platform
1. md5hash: network-id
1. md5hash: inode-of-etc
1. empty

### Platform

mac

### Network ID

MD5 of the 6 raw bytes of `IOMACAddress` on the Ethernet interface marked `IOPrimaryInterface` (the built-in port). Empty if there is none.

### inode of /etc

MD5 of the low 4 bytes of the inode number taken from `stat("/etc")`.

---
## Linux

Fields:
1. platform
1. md5hash: network-id
1. md5hash: inode-of-etc
1. empty
1. os-version

### Platform

linux

### Network ID

> On machines that name interfaces differently, the call fails and the result is an MD5 of a buffer of zeroes, returning `7319468847d7b1aee40dbf5dd963c999`.

MD5 of the 6-byte hardware address the `SIOCGIFHWADDR` ioctl returns for the interface named `eth0`.

### inode of /etc

MD5 of the low 4 bytes of the `/etc` inode number.
