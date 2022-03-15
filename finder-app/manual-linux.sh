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

echo "$OUTDIR"

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

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
    echo "mrproper: Remove all generated files + config +various backup files"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper
    echo "defconfig: Use the default configuration to build the kernel"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    echo "all: Build all"
    make -j4 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all
    echo "modules: compile modules"
    make -j4 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE modules
    echo "dtbs: build device tree(s)"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE dtbs
fi 

echo "Adding the Image in outdir"
cp -a $OUTDIR/linux-stable/arch/arm64/boot/Image $OUTDIR
#cp -a $OUTDIR/linux-stable/arch/arm64/boot/Image $OUTDIR/../aesd-autograder

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
cd "$OUTDIR"
echo "Creating Root Filesystem"
mkdir -p "$OUTDIR"/rootfs
cd "$OUTDIR"/rootfs
mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir usr/bin usr/lib usr/sbin
mkdir -p var/log




cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "Configuring busybox"
	make distclean
	make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox 
echo "Installing busybox"

make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE 
#make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE CONFIG_PREFIX="$OUTDIR"/rootfs ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE install #can be /rootfs/bin
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE CONFIG_PREFIX="$OUTDIR"/rootfs install
cd "$OUTDIR"/rootfs



echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
export SYSROOT=$(aarch64-none-linux-gnu-gcc -print-sysroot)

#cd "$OUTDIR"/rootfs

sudo cp -aL $SYSROOT/lib/ld-linux-aarch64.so.1 lib
#sudo cp -aL $SYSROOT/lib/../lib64/ld-2.31.so lib
sudo cp -aL $SYSROOT/lib64/libc.so.6 lib64
#sudo cp -aL $SYSROOT/lib64/libc-2.31.so lib64
sudo cp -aL $SYSROOT/lib64/libm.so.6 lib64
#sudo cp -aL $SYSROOT/lib64/libm-2.31.so lib64
sudo cp -aL $SYSROOT/lib64/libresolv.so.2 lib64
#sudo cp -aL $SYSROOT/lib64/libresolv-2.31.so lib64



# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
echo "DONE"

echo "Installing modules"
cd "$OUTDIR"/linux-stable
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE INSTALL_MOD_PATH="{OUTDIR}/rootfs" modules_install
# TODO: Clean and build the writer utility


cd ${FINDER_APP_DIR}
#make clean
#make CROSS_COMPILE
pwd
echo "${FINDER_APP_DIR}"
make clean
make CROSS_COMPILE=${CROSS_COMPILE}


#cp -a "/home/anshul/aesd/assignment-Anshul-99/finder-app/makefile" "$OUTDIR"/rootfs/home
cp writer "$OUTDIR"/rootfs/home
#"/home/anshul/aesd/assignment-Anshul-99/finder-app/writer"


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp finder.sh "$OUTDIR"/rootfs/home #"/home/anshul/aesd/assignment-Anshul-99/finder-app/finder.sh"
cp finder-test.sh "$OUTDIR"/rootfs/home #"/home/anshul/aesd/assignment-Anshul-99/finder-app/finder-test.sh"
cp autorun-qemu.sh "$OUTDIR"/rootfs/home #"/home/anshul/aesd/assignment-Anshul-99/finder-app/autorun-qemu.sh"
#cp start-qemu-terminal.sh "$OUTDIR"/rootfs/home #"/home/anshul/aesd/assignment-Anshul-99/finder-app/start-qemu-terminal.sh"
cp writer.c ${OUTDIR}/rootfs/home
cp writer ${OUTDIR}/rootfs/home
#mkdir conf
cp -Lr conf "$OUTDIR"/rootfs/home #"/home/anshul/aesd/assignment-Anshul-99/conf/username.txt"

# TODO: Chown the root directory
echo "Make the contents owned by root"

cd "$OUTDIR"/rootfs
sudo chown -R root:root *
sudo chown root:root ../rootfs

# TODO: Create initramfs.cpio.gz
cd "$OUTDIR"/rootfs
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd "$OUTDIR"
gzip initramfs.cpio
mkimage -A arm64 -O linux -T ramdisk -d initramfs.cpio.gz uRamdisk
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}





