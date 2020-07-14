#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eux

case "$1" in
	build)
		nproc && grep Mem /proc/meminfo && df -hT .
		apk add build-base bison findutils flex openssl-dev perl
		make montana_defconfig
		make KCFLAGS="-Wall -Werror" -j$(nproc)
		;;
	check)
		apk add git perl
		git format-patch origin/$DRONE_TARGET_BRANCH
		scripts/checkpatch.pl --strict --no-signoff --color=always *.patch || :
		! scripts/checkpatch.pl --strict --no-signoff --terse *.patch | grep -qF ERROR:
		;;
	*)
		exit 1
		;;
esac
