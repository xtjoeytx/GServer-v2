FROM amigadev/crosstools:x86_64-linux AS build-env
ARG VER_EXTRA
ARG TARGETARCH

# - ROOT -
USER 0

ENV VCPKG_ROOT=/tmp/gserver/vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_DISABLE_METRICS=1

COPY --chown=1001:1001 ./ /tmp/gserver

RUN ARCH=`echo $TARGETARCH| sed "s/amd64/x64/g" | sed "s/aarch64/arm64/g"` \
    && apt update \
    && apt install -y libssl-dev libzstd-dev cmake git ninja-build openjdk-17-jre gcc g++ \
    && git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT \
	&& cd $VCPKG_ROOT \
	&& sh bootstrap-vcpkg.sh -disableMetrics \
	&& cd /tmp/gserver \
	&& cmake -GNinja -S/tmp/gserver -B/tmp/gserver/build --preset "Release x64" -DVCPKG_TARGET_TRIPLET:STRING=${ARCH}-linux -DSTATIC=ON -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=ON -DUPNP=OFF -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
	&& cmake --build /tmp/gserver/build --target clean \
	&& cmake --build /tmp/gserver/build --target package --parallel $(getconf _NPROCESSORS_ONLN) \
	&& rm -rf /tmp/gserver/dist/_CPack_Packages \
	&& chown 1001:1001 -R /tmp/gserver \
	&& chmod 777 -R /tmp/gserver/dist \
	&& chmod 777 -R /tmp/gserver/build \
	&& apt purge -y libssl-dev libzstd-dev cmake git ninja-build openjdk-21-jre

# GServer Run Environment
FROM alpine:3.22
ARG CACHE_DATE=2025-07-08
COPY --from=build-env --chown=1001:1001 /tmp/gserver/dist /dist
COPY --from=build-env --chown=1001:1001 /tmp/gserver/build /tmp/gserver/build
RUN apk add --update libstdc++ libatomic cmake
USER 1001
WORKDIR /gserver
