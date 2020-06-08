#!/bin/bash 

set -eux

SCRIPT_FOLDER=`dirname $(readlink -f $0)`
REPO_ROOT="${SCRIPT_FOLDER}"

DEKEN_USER=avilleret
DEKEN_PACKAGE_NAME=mpv

if [[ "$CI_COMMIT_TAG" != "" ]]; then

  cd $REPO_ROOT
  # decrypt GPG key
  openssl aes-256-cbc -d -salt -pbkdf2 -in private-key.enc -out ${REPO_ROOT}/private-key -k ${PUREDATAINFO_PASSWORD}

  function cleanup {
    rm ${REPO_ROOT}/private-key
  }

  trap cleanup EXIT

  gpg --batch --import ${REPO_ROOT}/private-key

  GPG_COMMAND="gpg -ab --batch --yes "

  VERSION="test"
  if [[ "$CI_COMMIT_TAG" != "" ]]; then
    VERSION=${CI_COMMIT_TAG}
  else
    VERSION=git-${CI_COMMIT_SHORT_SHA}
  fi

  #Create folder on Webdav may fail if folder already exists
  curl --user ${DEKEN_USER}:${PUREDATAINFO_PASSWORD} -X MKCOL "https://puredata.info/Members/${DEKEN_USER}/software/${DEKEN_PACKAGE_NAME}/${VERSION}/" || true

upload_file() {
  #Upload source package
  mv ${ARCHIVE_SOURCE_FILE} ${ARCHIVE_NAME}
  read HASH FILE <<< `sha256sum "${ARCHIVE_NAME}"`
  echo $HASH > $FILE.sha256
  ${GPG_COMMAND} ${ARCHIVE_NAME}

  curl --user ${DEKEN_USER}:${PUREDATAINFO_PASSWORD}  -T "${ARCHIVE_NAME}"        "https://puredata.info/Members/${DEKEN_USER}/software/${DEKEN_PACKAGE_NAME}/${VERSION}/${ARCHIVE_NAME}" --basic
  curl --user ${DEKEN_USER}:${PUREDATAINFO_PASSWORD}  -T "${ARCHIVE_NAME}.sha256" "https://puredata.info/Members/${DEKEN_USER}/software/${DEKEN_PACKAGE_NAME}/${VERSION}/${ARCHIVE_NAME}.sha256" --basic
  curl --user ${DEKEN_USER}:${PUREDATAINFO_PASSWORD}  -T "${ARCHIVE_NAME}.asc"    "https://puredata.info/Members/${DEKEN_USER}/software/${DEKEN_PACKAGE_NAME}/${VERSION}/${ARCHIVE_NAME}.asc" --basic
}

  cd build/package

  ARCHIVE_NAME="./${DEKEN_PACKAGE_NAME}-v${VERSION}-(Sources)-externals.tar.gz"
  ARCHIVE_SOURCE_FILE="pd-*source.tar.gz"
  upload_file

  ARCHIVE_NAME="${DEKEN_PACKAGE_NAME}-v${VERSION}-(Linux-amd64-64)-externals.tar.gz"
  ARCHIVE_SOURCE_FILE="pd-*linux.tar.gz"
  upload_file

  cd ../../build-macos/package

  ARCHIVE_NAME="${DEKEN_PACKAGE_NAME}-v${VERSION}-(Darwin-x86_64-64)-externals.tar.gz"
  ARCHIVE_SOURCE_FILE="pd-*.zip"
  upload_file

fi
