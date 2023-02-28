#!/bin/bash

# This script will build and install them via Spack from source
# because Hermes requires a very specific version and configuration options
# for each package.

set -x
set -e
set -o pipefail

# Change this especially when your $HOME doesn't have enough disk space. 
INSTALL_DIR="${HOME}/${LOCAL}"
SPACK_DIR=${INSTALL_DIR}/spack

echo "Installing dependencies at ${INSTALL_DIR}"
mkdir -p ${INSTALL_DIR}

# Spack
git clone https://github.com/spack/spack ${SPACK_DIR}
pushd ${SPACK_DIR}
git checkout v${SPACK_VERSION}
popd

set +x
. ${SPACK_DIR}/share/spack/setup-env.sh
set -x

# This will allow Spack to skip building some packages that are directly
# available from the system. For example, autoconf, cmake, m4, etc.
# Modify ci/pckages.yaml to skip building compilers or build tools via Spack.
cp scripts/ci/packages.yaml ${SPACK_DIR}/etc/spack/packages.yaml

# Install hermes_shm (needed for dependencies)
spack install hermes_shm

popd
popd
popd