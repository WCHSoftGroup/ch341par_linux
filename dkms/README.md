ch34x_pis dkms driver
===

the folder for dkms driver system.
verify on ubuntu 22.04 passed

### install steps

```
$ sudo mkdir -p /usr/src/ch34x_pis-1.5
$ sudo cp -rfv ./*  /usr/src/ch34x_pis-1.5
$ sudo dkms add -m ch34x_pis -v 1.5
$ sudo dkms build -m ch34x_pis -v 1.5
$ sudo dkms install -m ch34x_pis -v 1.5
```

you can check the driver module information, the below context is on my system
```
$ sudo modinfo /lib/modules/$(uname -r)/updates/dkms/ch34x_pis.ko
filename:       /lib/modules/5.15.0-112-generic/updates/dkms/ch34x_pis.ko
license:        GPL
description:    USB to multiple interface driver for ch341/ch347, etc.
author:         WCH
srcversion:     0CD64C978818F76122F3A87
alias:          usb:v1A86p55E7d*dc*dsc*dp*ic*isc*ip*in02*
alias:          usb:v1A86p55DEd*dc*dsc*dp*ic*isc*ip*in04*
alias:          usb:v1A86p55DDd*dc*dsc*dp*ic*isc*ip*in02*
alias:          usb:v1A86p55DBd*dc*dsc*dp*ic*isc*ip*in02*
alias:          usb:v1A86p5512d*dc*dsc*dp*ic*isc*ip*in*
depends:
retpoline:      Y
name:           ch34x_pis
vermagic:       5.15.0-112-generic SMP mod_unload modversions
sig_id:         PKCS#7
signer:         localhost.localdomain Secure Boot Module Signature key
sig_key:        0D:E5:6D:7E:42:A3:FD:EF:11:09:E2:3C:94:83:55:5F:39:03:9B:41
sig_hashalgo:   sha512
signature:      55:C9:D5:E8:D0:A2:0B:99:6C:1C:B2:2B:D5:8C:D9:DB:56:E3:53:5D:
		FC:D5:F3:10:B1:26:54:32:38:DB:00:53:55:D9:3E:B1:DB:1B:8B:10:
		2B:A5:3E:DD:12:33:F6:78:8C:82:57:DC:5D:8E:DE:D9:B3:49:77:3A:
		4F:6A:DF:62:5E:CE:09:BB:4F:57:E9:82:E7:AA:A6:B9:BB:6A:61:7B:
		41:1E:C1:FF:6C:AC:AD:D6:97:49:80:65:B3:69:8C:B2:E7:32:A9:E8:
		63:CB:55:D1:B0:CE:80:3E:5A:37:DE:9A:C5:6B:86:1A:3A:5A:E5:94:
		75:EF:5C:0E:37:C9:F7:D3:24:80:B8:A8:EF:3D:7B:9B:7E:CB:9F:B5:
		9D:E2:45:65:2F:B6:92:4F:E0:54:3F:00:FF:2D:33:6A:E5:E7:69:DC:
		F2:1E:D1:C9:80:53:C5:C3:5E:62:D8:82:31:DB:C4:35:A2:01:EA:D2:
		E0:76:2F:2F:9C:85:36:5A:C4:6D:11:25:42:29:DA:B9:F9:CF:F3:2D:
		30:40:1A:FD:89:22:CE:4A:4E:48:4D:AC:CE:4E:8B:39:81:7B:E9:9F:
		E3:C5:0D:CA:98:09:D0:E4:62:26:84:6F:1B:9F:E6:F4:0F:86:30:C3:
		7A:58:39:39:7B:2F:D8:5B:CD:64:43:D3:FA:CA:EC:F1
```

