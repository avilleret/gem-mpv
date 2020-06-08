#!/bin/bash -ex

SCRIPT_FOLDER=`dirname $(readlink -f $0)`
REPO_ROOT="${SCRIPT_FOLDER}/.."
BUILD_FOLDER="${REPO_ROOT}/build"
PACKAGE_NAME=pd-mpv

pushd ${REPO_ROOT}
if [ -z $CI_COMMIT_TAG ]
then
  TIMESTAMP=`date +%Y.%m.%d-%H.%M.%S`
  HASH=`git describe --always || echo NO_HASH`
  VERSION=${TIMESTAMP}-${HASH}
else
  VERSION=$CI_COMMIT_TAG
fi
echo "Building Version: ${VERSION}"
popd

PD_PACKAGE_ARCHIVE=${BUILD_FOLDER}/package/${PACKAGE_NAME}-${VERSION}-linux.tar.gz

rm -rf ${BUILD_FOLDER}
mkdir -p ${BUILD_FOLDER} && cd ${BUILD_FOLDER}

cmake ${REPO_ROOT} -GNinja \
  -DCMAKE_C_COMPILER=/usr/bin/gcc-8 \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-8 \
  -DCMAKE_INSTALL_PREFIX=/usr

cmake --build .

cmake --build . --target install
if [ -d package ]; then
  pushd package
  tar cazf ${PD_PACKAGE_ARCHIVE} *
  popd
fi

pushd /tmp
  git clone $(realpath ${REPO_ROOT}) --recursive
  REPO_NAME=$(basename -- "$(realpath $REPO_ROOT)")
  tar -czf ${BUILD_FOLDER}/package/${PACKAGE_NAME}-source.tar.gz --exclude .git ${REPO_NAME}
  rm -rf {REPO_NAME}
popd

if [ ! -z $CI_COMMIT_TAG ]
then
  set +x # do not show FTP_* variable on command line
  curl -T "${PD_PACKAGE_ARCHIVE}" ftp://${FTP_USER}:${FTP_PASSWORD}@${FTP_URL}/releases/
fi

chown -R gitlab-runner ${BUILD_FOLDER}
chgrp -R gitlab-runner ${BUILD_FOLDER}