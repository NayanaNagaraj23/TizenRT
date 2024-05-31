/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/// @file tc_fs_mops.c

/// @brief Test Case for Filesystem Mountpoint Operation

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <tinyara/fs/ioctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <dirent.h>
#include "tc_common.h"
#include "tc_internal.h"
#include <tinyara/fs/ramdisk.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define TMPFS_TEST_CONTENTS "THIS IS FS MOPS"
#define TMPFS_TEST_MOUNTPOINT "/tmpfs_test"
#define ROMFS_TEST_MOUNTPOINT "/rom"
#define SMARTFS_TEST_MOUNTPOINT "/smartfs_test"
#define ROMFS_TEST_FILEPATH "/rom/init.d/rcS"
#define ROMFS_MOUNT_DEV_DIR "/dev/ram0"

#ifdef CONFIG_AUTOMOUNT_USERFS
static char *TMP_MOUNT_DEV_DIR;
#else
#define TMP_MOUNT_DEV_DIR "/dev/smart1"
#endif

#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
#define SMARTFS_MOUNT_DEV_DIR TMP_MOUNT_DEV_DIR"d1"
#else
#define SMARTFS_MOUNT_DEV_DIR TMP_MOUNT_DEV_DIR
#endif

static const unsigned char romfs_img[] = {
	0x2d, 0x72, 0x6f, 0x6d, 0x31, 0x66, 0x73, 0x2d, 0x00, 0x00, 0x02, 0x10,
	0xad, 0x5b, 0x65, 0xae, 0x4e, 0x53, 0x48, 0x49, 0x6e, 0x69, 0x74, 0x56,
	0x6f, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49,
	0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0xd1, 0xff, 0xff, 0x97,
	0x2e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x20,
	0x00, 0x00, 0x00, 0x00, 0xd1, 0xd1, 0xff, 0x80, 0x2e, 0x2e, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
	0x68, 0x2d, 0x96, 0x03, 0x69, 0x6e, 0x69, 0x74, 0x2e, 0x64, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0xd1, 0xd1, 0xff, 0x40,
	0x2e, 0x2e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x60,
	0x00, 0x00, 0x00, 0x00, 0xd1, 0xff, 0xfe, 0xe0, 0x2e, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x29,
	0x8d, 0x9c, 0xab, 0xcd, 0x72, 0x63, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x63, 0x68, 0x6f,
	0x20, 0x22, 0x56, 0x65, 0x72, 0x73, 0x61, 0x74, 0x69, 0x6c, 0x65, 0x20,
	0x53, 0x65, 0x6e, 0x73, 0x6f, 0x72, 0x20, 0x4e, 0x6f, 0x64, 0x65, 0x20,
	0x56, 0x31, 0x2e, 0x32, 0x2c, 0x20, 0x77, 0x77, 0x77, 0x2e, 0x6e, 0x65,
	0x74, 0x43, 0x6c, 0x61, 0x6d, 0x70, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x22,
	0x0a, 0x0a, 0x23, 0x20, 0x43, 0x72, 0x65, 0x61, 0x74, 0x65, 0x20, 0x61,
	0x20, 0x52, 0x41, 0x4d, 0x44, 0x49, 0x53, 0x4b, 0x20, 0x61, 0x6e, 0x64,
	0x20, 0x6d, 0x6f, 0x75, 0x6e, 0x74, 0x20, 0x69, 0x74, 0x20, 0x61, 0x74,
	0x20, 0x2f, 0x74, 0x6d, 0x70, 0x0a, 0x23, 0x6d, 0x6b, 0x72, 0x64, 0x20,
	0x2d, 0x6d, 0x20, 0x31, 0x20, 0x2d, 0x73, 0x20, 0x35, 0x31, 0x32, 0x20,
	0x34, 0x30, 0x0a, 0x23, 0x6d, 0x6b, 0x66, 0x61, 0x74, 0x66, 0x73, 0x20,
	0x2f, 0x64, 0x65, 0x76, 0x2f, 0x72, 0x61, 0x6d, 0x31, 0x0a, 0x23, 0x6d,
	0x6f, 0x75, 0x6e, 0x74, 0x20, 0x2d, 0x74, 0x20, 0x76, 0x66, 0x61, 0x74,
	0x20, 0x2f, 0x64, 0x65, 0x76, 0x2f, 0x72, 0x61, 0x6d, 0x31, 0x20, 0x2f,
	0x74, 0x6d, 0x70, 0x0a, 0x0a, 0x6d, 0x6f, 0x75, 0x6e, 0x74, 0x20, 0x2d,
	0x74, 0x20, 0x62, 0x69, 0x6e, 0x66, 0x73, 0x20, 0x2f, 0x64, 0x65, 0x76,
	0x2f, 0x72, 0x61, 0x6d, 0x30, 0x20, 0x2f, 0x73, 0x62, 0x69, 0x6e, 0x0a,
	0x0a, 0x72, 0x61, 0x6d, 0x74, 0x72, 0x6f, 0x6e, 0x20, 0x73, 0x74, 0x61,
	0x72, 0x74, 0x20, 0x33, 0x0a, 0x6d, 0x6f, 0x75, 0x6e, 0x74, 0x20, 0x2d,
	0x74, 0x20, 0x76, 0x66, 0x61, 0x74, 0x20, 0x2f, 0x64, 0x65, 0x76, 0x2f,
	0x6d, 0x74, 0x64, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x30, 0x20, 0x2f, 0x75,
	0x73, 0x72, 0x0a, 0x0a, 0x73, 0x64, 0x63, 0x61, 0x72, 0x64, 0x20, 0x73,
	0x74, 0x61, 0x72, 0x74, 0x20, 0x30, 0x0a, 0x6d, 0x6f, 0x75, 0x6e, 0x74,
	0x20, 0x2d, 0x74, 0x20, 0x76, 0x66, 0x61, 0x74, 0x20, 0x2f, 0x64, 0x65,
	0x76, 0x2f, 0x6d, 0x6d, 0x63, 0x73, 0x64, 0x30, 0x20, 0x2f, 0x73, 0x64,
	0x63, 0x61, 0x72, 0x64, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};

static void tc_fs_mops_mount(const char *filesystemtype)
{
	int ret = -1;

	if (strcmp(filesystemtype, "tmpfs") == 0) {
		ret = umount(TMPFS_TEST_MOUNTPOINT);
		ret = mount(NULL, TMPFS_TEST_MOUNTPOINT, "tmpfs", 0, NULL);
#ifndef CONFIG_BUILD_PROTECTED
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		ret = umount("/rom");
		romdisk_register(0, (FAR uint8_t *) romfs_img, 32, 512);
		ret = mount(ROMFS_MOUNT_DEV_DIR, ROMFS_TEST_MOUNTPOINT, "romfs", 1, NULL);

#endif
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		ret = umount(SMARTFS_TEST_MOUNTPOINT);
		ret = mount(SMARTFS_MOUNT_DEV_DIR, SMARTFS_TEST_MOUNTPOINT, "smartfs", 0, NULL);
	}

	TC_ASSERT_EQ("mount", ret, OK);
}

static void tc_fs_mops_open(const char *filesystemtype, int *fd)
{
	if (strcmp(filesystemtype, "tmpfs") == 0) {
		*fd = open(TMPFS_TEST_MOUNTPOINT "/test", O_RDWR | O_CREAT | O_TRUNC);
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		*fd = open(ROMFS_TEST_FILEPATH, O_RDONLY);
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		*fd = open(SMARTFS_TEST_MOUNTPOINT "/test", O_RDWR | O_CREAT | O_TRUNC);
	}

	TC_ASSERT_GEQ("open", fd, 0);
}

static void tc_fs_mops_write(const char *filesystemtype, int fd)
{
	int ret = -1;
	char *write_buf = TMPFS_TEST_CONTENTS;

	if (strcmp(filesystemtype, "romfs") == 0) {
		return;
	}

	ret = write(fd, write_buf, strlen(write_buf));
	TC_ASSERT_EQ_CLEANUP("write", ret, strlen(write_buf), close(fd));
}

static void tc_fs_mops_sync(const char *filesystemtype, int fd)
{
	int ret = -1;
	if (strcmp(filesystemtype, "smartfs") == 0) {
		ret = fsync(fd);
		TC_ASSERT_EQ_CLEANUP("fsync", ret, OK, close(fd));
	}
}

static void tc_fs_mops_lseek(const char *filesystemtype, int fd)
{
	int ret = -1;
	ret = lseek(fd, 0, SEEK_SET);
	TC_ASSERT_EQ_CLEANUP("lseek", ret, OK, close(fd));
}

static void tc_fs_mops_read(const char *filesystemtype, int fd)
{
	int ret = -1;
	char read_buf[32];
	char rom_buf[1024];
	char *write_buf = TMPFS_TEST_CONTENTS;

	if (strcmp(filesystemtype, "romfs") == 0) {
		memset(rom_buf, 0, sizeof(rom_buf));
		ret = read(fd, rom_buf, sizeof(rom_buf));
		TC_ASSERT_EQ_CLEANUP("read", ret, strlen(rom_buf), close(fd));
	} else {
		memset(read_buf, 0, sizeof(read_buf));
		ret = read(fd, read_buf, sizeof(read_buf));
		TC_ASSERT_EQ_CLEANUP("read", ret, strlen(write_buf), close(fd));
		TC_ASSERT_EQ_CLEANUP("read", strcmp(read_buf, TMPFS_TEST_CONTENTS), 0, close(fd));
	}
}

static void tc_fs_mops_ioctl(const char *filesystemtype, int fd)
{
	int ret = -1;
	long size;

	if (strcmp(filesystemtype, "tmpfs") == 0 || strcmp(filesystemtype, "romfs") == 0) {
		ret = ioctl(fd, FIOC_MMAP, (unsigned long)&size);
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		return;
	}

	TC_ASSERT_EQ_CLEANUP("ioctl", ret, OK, close(fd));
}

static void tc_fs_mops_dup(int fd)
{
	int fd_dup = -1;

	fd_dup = dup(fd);
	TC_ASSERT_GEQ_CLEANUP("dup", fd_dup, 0, close(fd));

	close(fd_dup);
}

static void tc_fs_mops_fstat(const char *filesystemtype, int fd)
{
	struct stat st;
	int ret = -1;

	ret = fstat(fd, &st);
	TC_ASSERT_EQ_CLEANUP("fstat", ret, OK, close(fd));
}

static void tc_fs_mops_opendir(const char *filesystemtype, DIR ** dir)
{

	if (strcmp(filesystemtype, "tmpfs") == 0) {
		*dir = opendir(TMPFS_TEST_MOUNTPOINT);
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		*dir = opendir("/rom/init.d");
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		*dir = opendir(SMARTFS_TEST_MOUNTPOINT);
	}

	TC_ASSERT_NEQ("opendir", *dir, NULL);
}

static void tc_fs_mops_readdir(DIR * dirp)
{
	struct dirent *dirent;

	dirent = readdir(dirp);
	TC_ASSERT_NEQ_CLEANUP("opendir", dirent, NULL, closedir(dirp));
}

static void tc_fs_mops_rewinddir(DIR * dirp)
{
	rewinddir(dirp);
}

static void tc_fs_mops_closedir(const char *filesystemtype, DIR * dir)
{
	int ret = -1;

	if (strcmp(filesystemtype, "romfs") == 0 || strcmp(filesystemtype, "smartfs") == 0) {
		return;
	}

	ret = closedir(dir);
	TC_ASSERT_EQ("closedir", ret, OK);
}

static void tc_fs_mops_mkdir(const char *filesystemtype)
{
	int ret = -1;

	if (strcmp(filesystemtype, "tmpfs") == 0) {
		ret = mkdir(TMPFS_TEST_MOUNTPOINT "/dir", 0777);
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		return;
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		ret = mkdir(SMARTFS_TEST_MOUNTPOINT "/dir", 0777);
	}

	TC_ASSERT_EQ("mkdir", ret, OK);
}

static void tc_fs_mops_rmdir(const char *filesystemtype)
{
	int ret = -1;

	if (strcmp(filesystemtype, "tmpfs") == 0) {
		ret = rmdir(TMPFS_TEST_MOUNTPOINT "/dir");
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		return;
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		ret = rmdir(SMARTFS_TEST_MOUNTPOINT "/dir");
	}

	TC_ASSERT_EQ("rmdir", ret, OK);
}

static void tc_fs_mops_stat(const char *filesystemtype)
{
	int ret = -1;
	struct stat st;
	char filename[100] = "/rom/init.d/rcS";

	if (strcmp(filesystemtype, "tmpfs") == 0) {
		ret = stat(TMPFS_TEST_MOUNTPOINT "/test", &st);
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		ret = stat(filename, &st);
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		ret = stat(SMARTFS_TEST_MOUNTPOINT "/test", &st);
	}

	TC_ASSERT_EQ("stat", ret, OK);
}

static void tc_fs_mops_rename(const char *filesystemtype)
{
	int ret = -1;

	if (strcmp(filesystemtype, "tmpfs") == 0) {
		ret = rename(TMPFS_TEST_MOUNTPOINT "/test", TMPFS_TEST_MOUNTPOINT "/test_new");
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		return;
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		ret = rename(SMARTFS_TEST_MOUNTPOINT "/test", SMARTFS_TEST_MOUNTPOINT "/test_new");
	}

	TC_ASSERT_EQ("rename", ret, OK);
}

static void tc_fs_mops_statfs(const char *filesystemtype)
{
	int ret = -1;
	struct statfs fs;

	if (strcmp(filesystemtype, "tmpfs") == 0) {
		ret = statfs(TMPFS_TEST_MOUNTPOINT, &fs);
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		ret = statfs("/rom", &fs);
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		ret = statfs(SMARTFS_TEST_MOUNTPOINT, &fs);
	}

	TC_ASSERT_EQ("statfs", ret, OK);
}

static void tc_fs_mops_unlink(const char *filesystemtype)
{
	int ret = -1;

	if (strcmp(filesystemtype, "tmpfs") == 0) {
		ret = unlink(TMPFS_TEST_MOUNTPOINT "/test_new");
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		return;
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		ret = unlink(SMARTFS_TEST_MOUNTPOINT "/test_new");
	}

	TC_ASSERT_EQ("unlink", ret, OK);
}

static void tc_fs_mops_umount(const char *filesystemtype)
{
	int ret = -1;

	if (strcmp(filesystemtype, "tmpfs") == 0) {
		ret = umount(TMPFS_TEST_MOUNTPOINT);
	} else if (strcmp(filesystemtype, "romfs") == 0) {
		ret = umount("/rom");
	} else if (strcmp(filesystemtype, "smartfs") == 0) {
		ret = umount(SMARTFS_TEST_MOUNTPOINT);
	}

	TC_ASSERT_EQ("umount", ret, OK);
}

static void tc_fs_mops_test_main(const char *filesystemtype)
{
	DIR *dir = NULL;
	int fd1 = -1;
	tc_fs_mops_mount(filesystemtype);
	tc_fs_mops_open(filesystemtype, &fd1);
	tc_fs_mops_write(filesystemtype, fd1);
	tc_fs_mops_sync(filesystemtype, fd1);
	tc_fs_mops_lseek(filesystemtype, fd1);
	tc_fs_mops_read(filesystemtype, fd1);
	tc_fs_mops_ioctl(filesystemtype, fd1);
	tc_fs_mops_dup(fd1);
	tc_fs_mops_fstat(filesystemtype, fd1);
	close(fd1);
	tc_fs_mops_opendir(filesystemtype, &dir);
	tc_fs_mops_readdir(dir);
	tc_fs_mops_rewinddir(dir);
	tc_fs_mops_closedir(filesystemtype, dir);
	tc_fs_mops_mkdir(filesystemtype);
	tc_fs_mops_rmdir(filesystemtype);
	tc_fs_mops_stat(filesystemtype);
	tc_fs_mops_rename(filesystemtype);
	tc_fs_mops_statfs(filesystemtype);
	tc_fs_mops_unlink(filesystemtype);
	tc_fs_mops_umount(filesystemtype);
	TC_SUCCESS_RESULT();
}

void tc_fs_mops_main(void)
{
#ifdef CONFIG_AUTOMOUNT_USERFS
	TMP_MOUNT_DEV_DIR = get_fs_mount_devname();
#endif
#ifdef CONFIG_TC_FS_TMPFS_MOPS
	tc_fs_mops_test_main("tmpfs");
#endif
#ifdef CONFIG_TC_FS_ROMFS_MOPS
#ifndef CONFIG_BUILD_PROTECTED
	tc_fs_mops_test_main("romfs");
#endif
#endif
#ifdef CONFIG_TC_FS_SMARTFS_MOPS
	tc_fs_mops_test_main("smartfs");
#endif
}
