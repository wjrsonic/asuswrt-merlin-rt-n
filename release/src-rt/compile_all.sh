#!/bin/sh

rm ./target.mak
cp ./target_full.mak ./target.mak
rm ./.config
make clean
./cmd_clean.sh
make rt-n15u
rm ./.config
make clean
./cmd_clean.sh
make rt-n10u
rm ./.config
make clean
make rt-n10p
./cmd_clean.sh
rm ./.config
make clean
./cmd_clean.sh
make rt-n53
rm ./.config
make clean
./cmd_clean.sh
make rt-n12d1
rm ./.config
make clean
./cmd_clean.sh
make rt-n12c1
rm ./.config
make clean
./cmd_clean.sh
make rt-n12b1
rm ./.config
make clean
./cmd_clean.sh
make rt-n12hp
rm ./.config
make clean
./cmd_clean.sh
make rt-n12hp_b1
rm ./.config
make clean
./cmd_clean.sh
make rt-n12vp
rm ./.config
make clean
./cmd_clean.sh
make rt-n14uhp
rm ./.config
make clean
./cmd_clean.sh
make rt-n16
rm ./target.mak
cp ./target_kit.mak ./target.mak
VER_EXT=`cat ./router/extendno.conf`
VER_EXT=$VER_EXT"-webkit"
echo $VER_EXT > ./router/extendno.conf
rm ./.config
make clean
./cmd_clean.sh
make rt-n15u
rm ./.config
make clean
./cmd_clean.sh
make rt-n10u
rm ./.config
make clean
./cmd_clean.sh
make rt-n14uhp

