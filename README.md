

# How to build:

In Linux Mint:
- sudo apt install tup g++ clang libboost-dev libsqlite3-dev libaudio-dev
- tup init
- tup

Then e.g.
- cd build-clang
- ./main

The "libaudio-dev" dependency comes from using Qt. The app does not play sounds.

