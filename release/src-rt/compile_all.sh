#!/bin/sh

rm ./.config
make clean
./cmd_clean.sh
make rt-n15u
sleep 180
rm ./.config
make clean
./cmd_clean.sh
make rt-n10u
sleep 180
rm ./.config
make clean
make rt-n10p
sleep 180
./cmd_clean.sh
rm ./.config
make clean
./cmd_clean.sh
make rt-n53
sleep 180
rm ./.config
make clean
./cmd_clean.sh
make rt-n12d1
sleep 180
rm ./.config
make clean
./cmd_clean.sh
make rt-n12c1
sleep 180
rm ./.config
make clean
./cmd_clean.sh
make rt-n12b1
sleep 180
rm ./.config
make clean
./cmd_clean.sh
make rt-n12hp
sleep 180
rm ./.config
make clean
./cmd_clean.sh
make rt-n12hp_b1
sleep 180
rm ./.config
make clean
./cmd_clean.sh
make rt-n12vp
sleep 180
rm ./.config
make clean
./cmd_clean.sh
make rt-n14uhp
sleep 180
rm ./.config
make clean
./cmd_clean.sh
make rt-n16
