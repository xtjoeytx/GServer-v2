ARG VER_EXTRA=""

# GServer Build Environment
FROM amigadev/crosstools:x86_64-w64-mingw32 AS build-env
ARG VER_EXTRA
ARG TARGETARCH

# - ROOT -
USER 0

ENV VCPKG_ROOT=/tmp/gserver/vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_DISABLE_METRICS=1

# Something is preventing this from being set via CMakePresets.json, so just force it.
ENV CMAKE_TOOLCHAIN_FILE=/tmp/gserver/vcpkg/scripts/buildsystems/vcpkg.cmake

COPY --chown=1001:1001 ./ /tmp/gserver

RUN ARCH=`echo $TARGETARCH| sed "s/amd64/x64/g" | sed "s/aarch64/arm64/g"` \
    && apt update \
    && apt install -y libssl-dev libzstd-dev cmake git ninja-build openjdk-21-jre \
    && ln -s /usr/x86_64-w64-mingw32/include/wincrypt.h /usr/x86_64-w64-mingw32/include/Wincrypt.h \
    && git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT \
	&& cd $VCPKG_ROOT \
	&& sh bootstrap-vcpkg.sh -disableMetrics \
	&& cd /tmp/gserver \
	&& cmake -GNinja -S/tmp/gserver -B/tmp/gserver/build --preset "Release x64" -DVCPKG_TARGET_TRIPLET:STRING=${ARCH}-mingw-static -DSTATIC=ON -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=ON -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
	&& cmake --build /tmp/gserver/build --target clean \
	&& cmake --build /tmp/gserver/build --target package --parallel $(getconf _NPROCESSORS_ONLN) \
	&& chmod 777 -R /tmp/gserver/dist \
	&& rm -rf /tmp/gserver/dist/_CPack_Packages \
	&& apt purge -y libssl-dev libzstd-dev cmake git ninja-build openjdk-21-jre

# GServer Run Environment
FROM alpine:3.22
ARG CACHE_DATE=2025-07-08
COPY --from=build-env --chown=1001:1001 /tmp/gserver/dist /dist
USER 1001
WORKDIR /dist
