#!/bin/sh

if [ "${TOPDIR}" = "" ]; then
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++"
    echo
    echo "\033[31mno configuration environment!!!!!!!!!!\033[0m"
    echo "eg: cd .. && source set_env"
    echo
    echo "+++++++++++++++++++++++++++++++++++++++++++++++++"
    exit 1
fi

make distclean
make imx_alientek_emmc_defconfig
make -j`nproc`

[ ! -d ${TOPDIR}/net_system/tftp ] && mkdir -p ${TOPDIR}/net_system/tftp
cp arch/arm/boot/zImage ${TOPDIR}/net_system/tftp
cp arch/arm/boot/dts/imx6ull-alientek-emmc.dtb ${TOPDIR}/net_system/tftp

