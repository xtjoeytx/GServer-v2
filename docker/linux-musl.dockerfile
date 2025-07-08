ARG VER_EXTRA=""

# GServer Build Environment
FROM alpine:3.22 AS build-env
ARG VER_EXTRA

# - ROOT -
USER 0

ENV VCPKG_ROOT=/tmp/gserver/vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_DISABLE_METRICS=1

COPY --chown=1001:1001 ./ /tmp/gserver

RUN apk add --update --virtual .gserver-build-dependencies \
		cmake \
        gcc \
        g++ \
        bison \
        flex \
        bash \
        make \
        git \
        curl \
        automake \
        autoconf \
        openjdk21-jdk \
        zip \
        ninja-build \
        ninja-is-really-ninja \
        openssl-dev \
        openssl-libs-static \
	&& git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT \
	&& cd $VCPKG_ROOT \
	&& sh bootstrap-vcpkg.sh -disableMetrics \
	&& cd /tmp/gserver \
	&& cmake -GNinja -S/tmp/gserver -B/tmp/gserver/build --preset "Release x64" -DVCPKG_TARGET_TRIPLET:STRING=x64-linux -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=ON -DUPNP=OFF -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
	&& cmake --build /tmp/gserver/build --config Release --target clean \
	&& cmake --build /tmp/gserver/build --config Release --target package --parallel $(getconf _NPROCESSORS_ONLN) \
	&& chmod 777 -R /tmp/gserver/dist \
	&& rm -rf /tmp/gserver/dist/_CPack_Packages \
    && chown 1001:1001 -R /tmp/gserver \
    && chmod 777 -R /tmp/gserver/build \
    && apk del --purge .gserver-build-dependencies

# - USER -
USER 1001

# GServer Run Environment
FROM alpine:3.22
ARG CACHE_DATE=2024-06-07
COPY --from=build-env /tmp/gserver/bin /gserver
COPY entrypoint.sh /gserver/
RUN apk add --update libstdc++ libatomic
WORKDIR /gserver
VOLUME /gserver/servers
ENTRYPOINT ["/gserver/entrypoint.sh"]
CMD ["/gserver/gs2emu"]
