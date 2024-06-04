#!/bin/sh

REPO_URL="https://github.com/Seeed-Studio/reCamera/releases/latest"
UPGRADE_FILE=rawimages.zip
BOOT_PARTITION=/dev/mmcblk0p1
RECV_PARTITION=/dev/mmcblk0p5
ROOTFS1=mmcblk0p3
ROOTFS2=mmcblk0p4
ROOTFS_FILE=rootfs_ext4.emmc

PERCENTAGE_FILE=/tmp/upgrade.percentage
CTRL_FILE=/tmp/upgrade.ctrl

PERCENTAGE=0

function clean_up() {
    rm -rf $CTRL_FILE
    umount $MOUNTPATH
    rm -rf $MOUNTPATH
}

function write_percent() {
    echo $PERCENTAGE > $PERCENTAGE_FILE

    if [ -f $CTRL_FILE ]; then
        stop_it=$(cat $CTRL_FILE)
        if [ "$stop_it" = "stop" ]; then
            echo "Stop upgrade."
            clean_up
            exit 2
        fi
    fi
}

function exit_upgrade() {
    write_percent
    clean_up
    exit $1
}

function write_upgrade_flag() {
    echo $1 > $MOUNTPATH/boot
    size=$(blockdev --getsize64 $BOOT_PARTITION)
    offset=$(expr $size / 512)
    offset=$(expr $offset - 1)
    dd if=$MOUNTPATH/boot of=$BOOT_PARTITION bs=512 seek=$offset count=1 conv=notrunc
}

case $1 in
start)
    if [ -z "$2" ]; then
        echo "Usage: $0 start <url>"
        exit 1
    fi
    REPO_URL=$2

    if [ -f $CTRL_FILE ]; then
        echo "Upgrade is running."
        exit 1
    fi
    echo "" > $CTRL_FILE

    PERCENTAGE=0
    write_percent
    echo "Step1: Check partition"
    fs_type=$(blkid -o value -s TYPE $RECV_PARTITION)
    if [ "$fs_type" = "ext4" ]; then
        PERCENTAGE=10
    else
        mkfs.ext4 $RECV_PARTITION
        # check again
        fs_type=$(blkid -o value -s TYPE $RECV_PARTITION)
        if [ "$fs_type" != "ext4" ]; then
            PERCENTAGE=10,"Recv partition is not ext4!"
            exit_upgrade 1
        fi
    fi

    write_percent
    echo "Step2: Mount partition"
    MOUNTPATH=$(mktemp -d)
    echo "Mount point: $MOUNTPATH"
    mount $RECV_PARTITION $MOUNTPATH
    if mount | grep -q "$RECV_PARTITION on $MOUNTPATH type"; then
        echo "Mount $RECV_PARTITION on $MOUNTPATH ok."
        PERCENTAGE=20
    else
        echo "Mount $RECV_PARTITION on $MOUNTPATH failed."
        PERCENTAGE=20,"Mount failed."
        exit_upgrade 1
    fi

    write_percent
    echo "Step3: Get upgrade url"
    REPO_URL=$(curl -skLi $REPO_URL | grep -i '^location:' | awk '{print $2}' | sed 's/^"//;s/"$//')
    echo "Repo url: $REPO_URL"
    if [ -z "$REPO_URL" ]; then
        PERCENTAGE=30,"Get repo url failed."
        exit_upgrade 1
    else
        PERCENTAGE=30
    fi
    full_url=$REPO_URL/$UPGRADE_FILE
    full_url=$(echo "$full_url" | sed 's/tag/download/g')
    echo "Full url: $full_url"

    write_percent
    echo "Step4: Download upgrade file"
    full_path=$MOUNTPATH/$UPGRADE_FILE
    echo "Full path: $full_path"
    rm -fv $full_path
    wget -q --no-check-certificate -O $full_path $full_url
    sync
    sleep 1
    if [ -f $full_path ]; then
        PERCENTAGE=40
    else
        PERCENTAGE=40,"Download failed."
        exit_upgrade 1
    fi

    write_percent
    echo "Step5: Check upgrade file"
    if [ -f $full_path ]; then
        PERCENTAGE=50
    else
        PERCENTAGE=50, "Package not found."
        exit_upgrade 1
    fi

    if unzip -l $full_path 2>&1 | grep -q "short read"; then
        PERCENTAGE=50,"Package corrupted."
        exit_upgrade 1
    else
        PERCENTAGE=50
    fi

    write_percent
    echo "Step6: Check $full_path md5"
    read_md5=$(unzip -p $full_path md5sum.txt | grep "$ROOTFS_FILE" | awk '{print $1}')
    echo "read_md5=$read_md5"
    file_md5=$(unzip -p $full_path $ROOTFS_FILE | md5sum | awk '{print $1}')
    echo "file_md5=$file_md5"
    if [ "$read_md5" == "$file_md5" ]; then
        PERCENTAGE=60
    else
        PERCENTAGE=60,"Package md5 check failed."
        exit_upgrade 1
    fi

    write_percent
    echo "Step7: Writing rootfs"
    rootfs1=$(lsblk 2>/dev/null | grep " /" | grep "$ROOTFS1")
    target=/dev/$ROOTFS2
    if [ -z "$rootfs1" ]; then
        rootfs2=$(lsblk 2>/dev/null | grep " /" | grep "$ROOTFS2")
        if [ -z "$rootfs2" ]; then
            PERCENTAGE=70, "Unknow rootfs partition."
            exit_upgrade 1
        else
            target=/dev/$ROOTFS1
        fi
    fi
    echo "target partition: $target"
    unzip -p $full_path $ROOTFS_FILE | dd of=$target bs=1M
    PERCENTAGE=70

    write_percent
    echo "Step8: Calc partition md5"
    partition_md5=$(dd if=$target bs=1M count=200 2>/dev/null | md5sum | awk '{print $1}')
    echo "partition_md5=$partition_md5"
    PERCENTAGE=80

    write_percent
    echo "Step9: Check partition md5"
    if [ "$partition_md5" == "$file_md5" ]; then
        PERCENTAGE=90
        if [ "$target" == "/dev/$ROOTFS1" ]; then
            write_upgrade_flag "rfs1"
        elif [ "$target" == "/dev/$ROOTFS2" ]; then
            write_upgrade_flag "rfs2"
        fi
    else
        PERCENTAGE=90,"Partition md5 check failed."
        exit_upgrade 1
    fi
    sync
    sleep 1

    PERCENTAGE=100
    echo "Finished!"
    exit_upgrade 0
    ;;

stop)
    echo "stop" > $CTRL_FILE
    ;;

query)
    if [ -f $PERCENTAGE_FILE ]; then
        cat $PERCENTAGE_FILE
    else
        echo "0"
    fi
    ;;

esac