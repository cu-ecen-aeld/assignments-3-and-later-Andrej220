#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
DIRLIST=( "bin" "dev" "etc" "home" "lib" "lib64" "proc" "sbin" "sys" "tmp" "usr" "var" "var/log" "usr/bin" "usr/lib" "usr/sbin" )

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}
if ! [ -d ${OUTDIR} ] ; then
        echo "${OUTDIR} could not be created"
	exit 1
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    # TODO: Add your kernel build steps here
    echo "Cleaning config"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    echo "Compiling all"
    make  -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    echo "Compiling modules"
    make  -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    echo "Compiling devices"
    make  -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
    cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
    echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
echo "Creating base directories"
for str in ${DIRLIST[@]}; do
	mkdir -p "${OUTDIR}/rootfs/${str}"
done

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    echo "Creating default configuration for BusyBox"
    make distclean
    make defconfig
else
    cd busybox
fi
# TODO: Make and install busybox
echo "Compiling  BusyBox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
echo "Installing BusyBox to ${OUTDIR}/rootfs"
make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install 

# TODO: Add library dependencies to rootfs
echo "Library dependencies"
file1=$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter")
files=$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library")

echo 'Copy interpreter' 
sysrootdir=$(${CROSS_COMPILE}gcc -print-sysroot)

while IFS= read -r libname ; do
    echo -en "\tCopying ---  ${libname} \t"
     cp ${sysrootdir}/lib/${libname} ${OUTDIR}/rootfs/lib/
    [ -f ${OUTDIR}/rootfs/lib/${libname} ] && echo "OK" || echo "Failed"
done <<< $(perl -ne 'print "$1\n" if /\/lib\/([a-zA-Z0-9\/\.-]+)/' <<<${file1} )

echo "Copy dependeses"
while IFS=  read -r libname ; do
    echo -en "\tCopying --- ${libname} \t"
     cp ${sysrootdir}/lib64/${libname} ${OUTDIR}/rootfs/lib64/
    [ -f ${OUTDIR}/rootfs/lib64/${libname} ] && echo "OK" || echo "Failed"
done <<< $(perl   -ne 'print "$1\n" if /\[([a-zA-Z0-9\/\.-]*)\]/' <<<${files})

# TODO: Make device nodes
cd ${OUTDIR}/rootfs
echo -ne "Creating devices \t"
sudo mknod -m 666 dev/null c 1 3 || echo "Failed to create dev/null"
sudo mknod -m 666 dev/console c 5 1 || echo "Failed to create dev/console"
[ -n "$?" ] && echo -e "\tOK" || echo -e "\tFailed"

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
echo "Cleaning wirter compilation"
make clean
echo "Making new writer"
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo -n "Copying fid-app to rootfs/home"
cp -r -L -f  ${FINDER_APP_DIR}/.  ${OUTDIR}/rootfs/home
[ -n $? ] && echo -e "\tOK"

# TODO: Chown the root directory
echo -n "Chaning roots owner to root"
cd  ${OUTDIR}/rootfs
sudo chown -R root:root ${OUTDIR}/rootfs
[ -n $? ] && echo -e "\tOK"
# TODO: Create initramfs.cpio.gz
echo -n "Creating initramfs.cpio"
sudo find  . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
[ -n $? ] && echo -e "\tOK"
echo -n "Compressing initramfs"
gzip -f ${OUTDIR}/initramfs.cpio
[ -n $? ] && echo -e "\tOK"

