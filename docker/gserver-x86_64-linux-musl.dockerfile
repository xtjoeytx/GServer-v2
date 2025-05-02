ARG VER_EXTRA=""

# GServer Build Environment
FROM alpine:3.20 AS build-env
ARG VER_EXTRA

RUN VCPKG_ROOT=/tmp/vcpkg \
	&& git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT \
	&& cd /tmp/vcpkg \
	&& sh bootstrap-vcpkg.sh

RUN apk add --update --virtual .gserver-build-dependencies \
		cmake \
		gcc \
		g++ \
		bison \
		flex \
		bash \
		make \
		git \
		automake \
		autoconf \
		ninja \
		openssl-dev \
		openssl-libs-static \
	&& cd /tmp/gserver \
	&& cmake -GNinja -S/tmp/gserver -B/tmp/gserver/build --preset vcpkg -DVCPKG_TARGET_TRIPLET:STRING=x64-linux -DCMAKE_BUILD_TYPE=Release -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=ON -DUPNP=OFF -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
	&& cmake --build /tmp/gserver/build --config Release --target clean \
	&& cmake --build /tmp/gserver/build --config Release --target package --parallel $(getconf _NPROCESSORS_ONLN) \
	&& chmod 777 -R /tmp/gserver/dist \
	&& rm -rf /tmp/gserver/dist/_CPack_Packages \
    && chown 1001:1001 -R /tmp/gserver \
    && chmod 777 -R /tmp/gserver/build \
    && apk del --purge .gserver-build-dependencies

USER 1001

# GServer Run Environment
FROM alpine:3.20
ARG CACHE_DATE=2024-06-07
COPY --from=build-env /tmp/gserver/bin /gserver
COPY entrypoint.sh /gserver/
RUN apk add --update libstdc++ libatomic
WORKDIR /gserver
VOLUME /gserver/servers
ENTRYPOINT ["/gserver/entrypoint.sh"]
CMD ["/gserver/gs2emu"]
