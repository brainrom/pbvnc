PBVnc
=========

VNC remote desktop client for PocketBook devices. 

Description
-----------

This app allows to connect to ordinary VNC servers in LAN without password,
primarly aimed to stream visual novells from PC.

Installation
-----------

1. Place `pbvnc.app` and `pbvnc.cfg` in `applictaions/` directory on your device's storage.
2. Adjust host address in `pbvnc.cfg`.

Building
-----------
Application can be built using `SDK_481` [PocketBook SDK](https://github.com/blchinezu/pocketbook-sdk) toolchain.
```
mkdir build
cd build
env LANG=C PB_SDK_CFG=~/pocketbook/SDK_481/bin cmake ..
env LANG=C cmake --build .
```
