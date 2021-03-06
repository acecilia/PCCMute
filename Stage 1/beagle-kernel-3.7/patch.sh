#!/bin/bash
# (c) 2009 - 2012 Koen Kooi <koen@dominion.thruhere.net>
# (c) 2012 Robert Nelson <robertcnelson@gmail.com>
# This script will take a set of directories with patches and make a git tree out of it
# After all the patches are applied it will output a SRC_URI fragment you can copy/paste into a recipe
set -e

# don't use builtin 'echo' from /bin/sh
export ECHO="$(which echo)"

DIR="$PWD"
PATCHPATH="${DIR}/patches"
EXPORTPATH="${DIR}/export"

RECIPEDIR="linux-mainline-3.7"
RECIPENAME="linux-mainline_3.7.bb"
RECIPEFILE="${DIR}/recipes/${RECIPENAME}"

#For TAG, use mainline Kernel tags
TAG="v3.7"
EXTRATAG=""

#EXTERNAL_TREE="git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git"
#EXTERNAL_BRANCH="master"
#EXTERNAL_SHA="29594404d7fe73cd80eaa4ee8c43dcc53970c60e"

EXTERNAL_TAG="v3.7.4"
EXTERNAL_TREE="git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git"
EXTERNAL_BRANCH="linux-3.7.y"
EXTERNAL_SHA="8a69ca20f06d1878aa0ee8000a60d8226ab6b6a1"

#PATCHSET="pruss pwm2 fixes"
# BAD: rtc
PATCHSET="dma cpsw pinctrl cpufreq adc pwm i2c usb da8xx-fb mmc fixes f2fs 6lowpan capebus hdmi pruss arm omap omap_sakoman omap_beagle_expansion omap_beagle omap_panda omap_sgx omap_fixes omap_thermal net spi capes"

git_kernel_stable () {
	git pull git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git master --tags || true
}

git_pull_torvalds () {
	git pull git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git master --tags || true
	#Maybe, we need a stable tag '3.0.2'?
	git tag | grep ${TAG} >/dev/null || git_kernel_stable
}

if [ ! -d ${DIR}/kernel ] ; then
	mkdir -p ${DIR}/kernel
fi

cd ${DIR}/kernel

if [ ! -f ./.git/config ] ; then
	git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git .
else
	git fetch
fi

git am --abort || echo "Do you need to make sure the patches apply cleanly first?"

# Always return to master, and remove patched branch...
git reset --hard
git checkout master -f
git describe
git branch -D tmp-patching-branch &>/dev/null || true
git branch -D tmp-patching-branch-sha &>/dev/null || true

#Do we have the tag?
git tag | grep ${TAG} | grep -v rc >/dev/null || git_pull_torvalds
git checkout -f ${TAG} -b tmp-patching-branch

if [ "${EXTERNAL_TREE}" ] ; then
	#we are pulling the  external tree into 1st branch, and checkout the SHA into a 2nd, 
	#which saves a little pain in cleaning up master, when switching between different beagleboard branches
	git pull ${EXTERNAL_TREE} ${EXTERNAL_BRANCH}
	git checkout ${EXTERNAL_SHA} -b tmp-patching-branch-sha
fi

git describe

if [ -d ${EXPORTPATH} ] ; then
	rm -rf ${EXPORTPATH} || true
	rm -rf ${EXPORTPATH}-oe || true
fi

# apply patches
for patchset in ${PATCHSET} ; do
	CURRENTCOMMIT="$(git log --oneline --no-abbrev -1 | awk '{print $1}')"

	mkdir -p ${EXPORTPATH}/$patchset
	for patch in $(ls -1 ${PATCHPATH}/$patchset/*.patch | sort -n) ; do
		$ECHO -n "$patch: "
		git am  -q $patch && echo applied || exit 1
	done

	NEWCOMMIT="$(git log --oneline --no-abbrev -1 | awk '{print $1}')"

	git format-patch ${CURRENTCOMMIT}..${NEWCOMMIT} --quiet -o ${EXPORTPATH}/$patchset
	rm -rf ${PATCHPATH}/$patchset && cp -a ${EXPORTPATH}/$patchset ${PATCHPATH}

	git commit --allow-empty -a -m "${TAG}-${patchset}${EXTRATAG}"
done

mkdir -p ${EXPORTPATH}-oe/recipes-kernel/linux
cp ${RECIPEFILE} ${EXPORTPATH}-oe/recipes-kernel/linux/

if [ "${EXTERNAL_TREE}" ] ; then
	echo "PV = \"${EXTERNAL_TAG}\"" >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
	echo >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
	echo 'SRCREV_pn-${PN}' \= \"${EXTERNAL_SHA}\" >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
	echo >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
	echo 'SRC_URI = " \' >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
	echo "	${EXTERNAL_TREE};branch=${EXTERNAL_BRANCH}"' \' >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
else
	echo "PV = \"3.7.0\"" >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
	echo >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
fi

if [ -f ${DIR}/patch_script.sh ] ; then
	rm -rf ${DIR}/patch_script.sh || true
fi

# export patches and output SRC_URI for them
for patchset in ${PATCHSET} ; do
	for patch in $(ls -1 ${EXPORTPATH}/$patchset/*.patch | sort -n) ; do
		patch=${patch##*/}
		echo -e "\tfile://${patchset}/$patch \\" >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
		echo "	git am \"\${DIR}/patches/${patchset}/$patch\"" >> ${DIR}/patch_script.sh
	done
done

echo '	file://defconfig \' >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}
echo "\"" >> ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPENAME}

mkdir -p ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPEDIR}
cp -a ${EXPORTPATH}/* ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPEDIR}/

if [ -e ${DIR}/kernel/.config ] ; then
	cp ${DIR}/kernel/.config ${EXPORTPATH}-oe/recipes-kernel/linux/${RECIPEDIR}/defconfig
fi
