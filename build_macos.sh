#!/bin/bash -ex

SCRIPT_FOLDER=`dirname $(greadlink -f $0)`
REPO_ROOT="${SCRIPT_FOLDER}"
BUILD_FOLDER="${REPO_ROOT}/build-macos"
PACKAGE_NAME=pd-mpv

if [ -z $CI_COMMIT_TAG ]
then
  TIMESTAMP=`date +%Y.%m.%d-%H.%M.%S`
  HASH=`git describe --always || echo NO_HASH`
  VERSION=${TIMESTAMP}-${HASH}
else
  VERSION=$CI_COMMIT_TAG
fi
echo "Building Version: ${VERSION}"

PD_PACKAGE_ARCHIVE=${BUILD_FOLDER}/package/${PACKAGE_NAME}-${VERSION}-macos.zip

rm -rf ${BUILD_FOLDER}
mkdir -p ${BUILD_FOLDER} && cd ${BUILD_FOLDER}
cmake .. -GNinja \
	-DPUREDATA_INCLUDE_DIRS=/Applications/Pd-0.50-2.app/Contents/Resources/src/ \
	-DGEM_INCLUDE_DIRS=${HOME}/Documents/Pd/externals/Gem/include/ \
	-DCMAKE_INSTALL_PREFIX=${PWD}/install

cmake --build .
cmake --build . --target install
cd package && tar cf ${PD_PACKAGE_ARCHIVE} *

if [ ! -z $CI_COMMIT_TAG ]
then
  set +x # do not show FTP_* variable on command line
  curl -T "${PD_PACKAGE_ARCHIVE}" ftp://${FTP_USER}:${FTP_PASSWORD}@${FTP_URL}/releases/
fi