## TODO: replace with base image created under NGWPC-3223 ##
## see: https://jira.nextgenwaterprediction.com/browse/NGWPC-3223
FROM rockylinux:8

# ensure local python is preferred over distribution python
ENV PATH="/usr/local/bin:$PATH"

# cannot remove LANG even though https://bugs.python.org/issue19846 is fixed
# last attempted removal of LANG broke many users:
# https://github.com/docker-library/python/pull/570
ENV LANG="C.UTF-8"

ENV PYTHON_VERSION="3.10.14"
ENV SZIP_VERSION="2.1.1"
ENV HDF5_VERSION="1.10.11"
ENV NETCDF_C_VERSION="4.7.4"
ENV NETCDF_FORTRAN_VERSION="4.5.4"
ENV BOOST_VERSION="1.79.0"

## FIXME: Replace installation and build of FOSS dependencies wiith a base image. ##

# runtime dependencies
RUN set -eux; \
    dnf install -y epel-release; \
    dnf config-manager --set-enabled powertools; \
    dnf install -y \
        cmake \
        curl curl-devel \
        file \
        findutils \
        git \
## FIXME: replace GNU compilers with Intel compiler ##
        gcc-toolset-10 \
        gcc-toolset-10-libasan-devel \
        libasan6 \
        libffi libffi-devel \
        m4 \
        udunits2 udunits2-devel \
        bzip2 bzip2-devel \
        zlib zlib-devel \
## FIXME: replace openmpi with Intel MPI libraries ##
        openmpi openmpi-devel \
        openssl openssl-devel \
        rsync \
        sqlite sqlite-devel \
        tk tk-devel \
        uuid uuid-devel \
        which \
        xz \
        jq \
    ; \
    dnf clean all

## FIXME: replace GNU compilers with Intel compiler ##
SHELL [ "/usr/bin/scl", "enable", "gcc-toolset-10" ]
## FIXME: replace openmpi with Intel MPI libraries ##
ENV PATH="${PATH}:/usr/lib64/openmpi/bin/"

# Fix OpenMPI support within container
ENV PSM3_HAL=loopback
ENV PSM3_DEVICES=self

RUN set -eux; \
	\
	curl --location --output python.tar.xz "https://www.python.org/ftp/python/${PYTHON_VERSION%%[a-z]*}/Python-$PYTHON_VERSION.tar.xz"; \
	mkdir --parents /usr/src/python; \
	tar --extract --directory /usr/src/python --strip-components=1 --file python.tar.xz; \
	rm python.tar.xz; \
	\
	cd /usr/src/python; \
	./configure \
		--enable-loadable-sqlite-extensions \
		--enable-optimizations \
		--enable-option-checking=fatal \
		--enable-shared \
		--with-lto \
		--with-system-expat \
		--without-ensurepip \
	; \
	nproc="$(nproc)"; \
	make -j "$nproc" \
		"PROFILE_TASK=${PROFILE_TASK:-}" \
	; \
# https://github.com/docker-library/python/issues/784
# prevent accidental usage of a system installed libpython of the same version
	rm python; \
	make -j "$nproc" \
		"LDFLAGS=${LDFLAGS:--Wl},-rpath='\$\$ORIGIN/../lib'" \
		"PROFILE_TASK=${PROFILE_TASK:-}" \
		python \
	; \
	make install; \
# enable GDB to load debugging data: https://github.com/docker-library/python/pull/701
    bin="$(readlink -ve /usr/local/bin/python3)"; \
    dir="$(dirname "$bin")"; \
    mkdir --parents "/usr/share/gdb/auto-load/$dir"; \
    cp -vL Tools/gdb/libpython.py "/usr/share/gdb/auto-load/$bin-gdb.py"; \
    \
    cd /; \
    rm -rf /usr/src/python; \
    \
    find /usr/local -depth \
        \( \
            \( -type d -a \( -name test -o -name tests -o -name idle_test \) \) \
            -o \( -type f -a \( -name '*.pyc' -o -name '*.pyo' -o -name 'libpython*.a' \) \) \
        \) -exec rm -rf '{}' + \
    ; \
    \
    ldconfig; \
    \
    python3 --version

# make some useful symlinks that are expected to exist ("/usr/local/bin/python" and friends)
RUN set -eux; \
	for src in idle3 pydoc3 python3 python3-config; do \
		dst="$(echo "$src" | tr -d 3)"; \
		[ -s "/usr/local/bin/$src" ]; \
		[ ! -e "/usr/local/bin/$dst" ]; \
		ln -svT "$src" "/usr/local/bin/$dst"; \
	done

RUN set -eux; \
	\
	curl --location --output szip.gz "https://docs.hdfgroup.org/archive/support/ftp/lib-external/szip/${SZIP_VERSION%%[a-z]*}/src/szip-${SZIP_VERSION}.tar.gz"; \
	mkdir --parents /usr/src/szip; \
	tar --extract --directory /usr/src/szip --strip-components=1 --file szip.gz; \
	rm szip.gz; \
	\
	cd /usr/src/szip; \
	./configure --prefix=/usr/local/ \
	; \
	nproc="$(nproc)"; \
	make -j "$nproc" \
	; \
	make install ; \
    \
    rm --recursive --force /usr/src/szip

RUN set -eux; \
	\
	curl --location --output libhdf5.tar.gz "https://github.com/HDFGroup/hdf5/archive/refs/tags/hdf5-${HDF5_VERSION//./_}.tar.gz"; \
	mkdir --parents /usr/src/hdf5; \
	tar --extract --directory /usr/src/hdf5 --strip-components=1 --file libhdf5.tar.gz; \
	rm libhdf5.tar.gz; \
	\
	cd /usr/src/hdf5; \
	./configure --prefix=/usr/local/ --with-szlib=/usr/local/ \
	; \
	nproc="$(nproc)"; \
	make check -j "$nproc" \
	; \
    make install ; \
    \
    rm --recursive --force /usr/src/hdf5

RUN set -eux; \
    \
    curl --location --output netcdf-c.tar.gz "https://github.com/Unidata/netcdf-c/archive/refs/tags/v${NETCDF_C_VERSION%%[a-z]*}.tar.gz"; \
    mkdir --parents /usr/src/netcdf-c; \
    tar --extract --directory /usr/src/netcdf-c --strip-components=1 --file netcdf-c.tar.gz; \
    rm netcdf-c.tar.gz; \
    \
    cd /usr/src/netcdf-c; \
    cmake -B cmake_build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DENABLE_TESTS=OFF \
    ; \
    nproc="$(nproc)"; \
    cmake --build cmake_build --parallel "$nproc" \
    ; \
    cmake --build cmake_build --target install ; \
    \
    rm --recursive --force /usr/src/netcdf-c

RUN set -eux; \
    \
    curl --location --output netcdf-fortran.tar.gz "https://github.com/Unidata/netcdf-fortran/archive/refs/tags/v${NETCDF_FORTRAN_VERSION%%[a-z]*}.tar.gz"; \
    mkdir --parents /usr/src/netcdf-fortran; \
    tar --extract --directory /usr/src/netcdf-fortran --strip-components=1 --file netcdf-fortran.tar.gz; \
    rm netcdf-fortran.tar.gz; \
    \
    cd /usr/src/netcdf-fortran; \
    cmake -B cmake_build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DENABLE_TESTS=OFF \
    -DCMAKE_Fortran_COMPILER=/opt/rh/gcc-toolset-10/root/usr/bin/gfortran \
    ; \
    nproc="$(nproc)"; \
    cmake --build cmake_build --parallel "$nproc" \
    ; \
    cmake --build cmake_build --target install ; \
    \
    rm --recursive --force /usr/src/netcdf-fortran

RUN set -eux; \
    \
    curl --location --output boost.tar.gz "https://archives.boost.io/release/${BOOST_VERSION%%[a-z]*}/source/boost_${BOOST_VERSION//./_}.tar.gz"; \
    mkdir --parents /opt/boost; \
    tar --extract --directory /opt/boost --strip-components=1 --file boost.tar.gz; \
    rm boost.tar.gz

COPY . /ngen-app/ngen/

ENV VIRTUAL_ENV=/ngen-app/ngen-python
RUN set -eux; \
	\
    python3.10 -m venv ${VIRTUAL_ENV}
ENV PATH=${VIRTUAL_ENV}/bin:${PATH}

WORKDIR /ngen-app/

RUN set -eux; \
	\
    pip3 install -r ngen/extern/test_bmi_py/requirements.txt; \
    pip3 install -r ngen/extern/t-route/requirements.txt ; \
    # Lock numpy and netcdf4 versions so t-route doesn't break
    pip3 install "numpy==1.26.4" "netcdf4<=1.6.3" ; \
    pip3 cache purge

RUN set -eux; \
	\
    cd ngen/extern/t-route ; \
    LDFLAGS="-Wl,-L/usr/local/lib64/,-L/usr/local/lib/,-rpath,/usr/local/lib64/,-rpath,/usr/local/lib/" ./compiler.sh no-e ; \
    \
    pip3 cache purge

WORKDIR /ngen-app/ngen/
RUN set -eux; \
    cmake -B cmake_build -S . \
## FIXME: figure out why running with MPI enabled throws errors
##      and re-enable it.
        -DNGEN_WITH_MPI=ON \
        -DNGEN_WITH_NETCDF=ON \
        -DNGEN_WITH_SQLITE=ON \
        -DNGEN_WITH_UDUNITS=ON \
        -DNGEN_WITH_BMI_FORTRAN=ON \
        -DNGEN_WITH_BMI_C=ON \
        -DNGEN_WITH_PYTHON=ON \
        -DNGEN_WITH_TESTS=ON \
        -DNGEN_WITH_ROUTING=ON \
        -DNGEN_QUIET=ON \
        -DNGEN_UPDATE_GIT_SUBMODULES=OFF \
        -DBOOST_ROOT=/opt/boost/; \
    nproc="$(nproc)"; \
    cmake --build cmake_build --target all --parallel ${nproc} ; \
    \
    cmake -B extern/LASAM/cmake_build -S extern/LASAM/ -DNGEN=ON ; \
    cmake --build extern/LASAM/cmake_build/ ; \
    \
    cmake -B extern/snow17/cmake_build -S extern/snow17/ ; \
    cmake --build extern/snow17/cmake_build/ ; \
    \
    cmake -B extern/sac-sma/cmake_build -S extern/sac-sma/ ; \
    cmake --build extern/sac-sma/cmake_build/ ; \
    \
    cmake -B extern/SoilMoistureProfiles/cmake_build -S extern/SoilMoistureProfiles/SoilMoistureProfiles/ -DNGEN=ON ; \
    cmake --build extern/SoilMoistureProfiles/cmake_build/ ; \
    \
    cmake -B extern/SoilFreezeThaw/cmake_build -S extern/SoilFreezeThaw/SoilFreezeThaw/ -DNGEN=ON ; \
    cmake --build extern/SoilFreezeThaw/cmake_build/ ; \
    \
    cmake -B extern/ueb-bmi/cmake_build -S extern/ueb-bmi/ -DBMICXX_INCLUDE_DIRS=/ngen-app/ngen/extern/bmi-cxx/ ; \
    cmake --build extern/ueb-bmi/cmake_build/

RUN set -eux; \
    mkdir --parents /ngencerf/data/ngen-run-logs/ ; \
    mkdir --parents /ngen-app/bin/ ; \
    mv run-ngen.sh /ngen-app/bin/ ; \
    chmod +x /ngen-app/bin/run-ngen.sh

WORKDIR /ngen-app/ngen

# --- Git Info Extraction with Submodules ---
# Extract Git information for the main repository
ARG CI_COMMIT_REF_NAME

RUN set -eux; \
    # Get the remote URL from Git configuration
    repo_url=$(git config --get remote.origin.url); \
    # Extract the repo name (everything after the last slash) and remove any trailing .git
    key=${repo_url##*/}; \
    key=${key%.git}; \
    # Construct the file path using the derived key
    export GIT_INFO_PATH="/ngen-app/${key}_git_info.json"; \
    # Determine branch name: use CI_COMMIT_REF_NAME if set; otherwise, use git's current branch
    branch=$( [ -n "${CI_COMMIT_REF_NAME:-}" ] && echo "${CI_COMMIT_REF_NAME}" || git rev-parse --abbrev-ref HEAD ); \
    jq -n \
      --arg commit_hash "$(git rev-parse HEAD)" \
      --arg branch "$branch" \
      --arg tags "$(git tag --points-at HEAD | tr '\n' ' ')" \
      --arg author "$(git log -1 --pretty=format:'%an')" \
      --arg commit_date "$(date -u -d @$(git log -1 --pretty=format:'%ct') +'%Y-%m-%d %H:%M:%S UTC')" \
      --arg message "$(git log -1 --pretty=format:'%s' | tr '\n' ';')" \
      --arg build_date "$(date -u +'%Y-%m-%d %H:%M:%S UTC')" \
      "{\"$key\": {commit_hash: \$commit_hash, branch: \$branch, tags: \$tags, author: \$author, commit_date: \$commit_date, message: \$message, build_date: \$build_date}}" \
      > $GIT_INFO_PATH; \
    \
    # Create directory for submodule JSON files
    mkdir -p /ngen-app/submodules-json; \
    \
    # Process each submodule listed in .gitmodules (skipping unwanted ones)
    for sub in $(git config --file .gitmodules --get-regexp path | awk '{print $2}'); do \
      cd "$sub"; \
      # Derive submodule key from its remote URL
      subrepo_url=$(git config --get remote.origin.url); \
      # Remove trailing slash if present
      subrepo_url=${subrepo_url%/}; \
      sub_key=${subrepo_url##*/}; \
      sub_key=${sub_key%.git}; \
      \
      echo sub_key $sub_key; \
      # Skip unwanted submodules based on the derived key
      if [[ "$sub_key" == "googletest" || "$sub_key" == "pybind11" || "$sub_key" == "netcdf-cxx4" ]]; then \
        cd - > /dev/null; \
        continue; \
      fi; \
      \
      info=$(jq -n \
        --arg commit_hash "$(git rev-parse HEAD)" \
        --arg branch "$(git rev-parse --abbrev-ref HEAD)" \
        --arg tags "$(git tag --points-at HEAD | tr '\n' ' ')" \
        --arg author "$(git log -1 --pretty=format:%an)" \
        --arg commit_date "$(date -u -d @$(git log -1 --pretty=format:%ct) +'%Y-%m-%d %H:%M:%S UTC')" \
        --arg message "$(git log -1 --pretty=format:%s | tr '\n' ' ')" \
        --arg build_date "$(date -u +'%Y-%m-%d %H:%M:%S UTC')" \
        '{"'"$sub_key"'": {commit_hash: $commit_hash, branch: $branch, tags: $tags, author: $author, commit_date: $commit_date, message: $message, build_date: $build_date}}' ); \
      cd - > /dev/null; \
      echo "$info" > /ngen-app/submodules-json/git_info_"$sub_key".json; \
    done; \
    \
    # Merge the main repository JSON with all submodule JSON files as top-level objects
    jq -s 'add' $GIT_INFO_PATH /ngen-app/submodules-json/*.json > /ngen-app/merged_git_info.json && \
    mv /ngen-app/merged_git_info.json $GIT_INFO_PATH && \
    rm -rf /ngen-app/submodules-json


WORKDIR /
SHELL ["/bin/bash", "-c"]

ENTRYPOINT [ "/ngen-app/bin/run-ngen.sh" ]
CMD [ "--info" ]

