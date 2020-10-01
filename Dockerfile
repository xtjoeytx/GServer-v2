FROM xtjoeytx/v8:7.4.288.26 as v8

# GServer Build Environment
FROM alpine:3.10 AS build-env
COPY ./ /gserver
COPY --from=v8 /tmp/v8 /gserver/dependencies/v8
RUN apk add --update --virtual .gserver-build-dependencies \
		cmake \
		gcc \
		g++ \
		bash \
		make \
		git \
		automake \
		autoconf \
	&& mkdir /gserver/build \
	&& cd /gserver/build \
	&& cmake .. -DCMAKE_BUILD_TYPE=Release -DV8NPCSERVER=ON -DNOUPNP=ON -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
	&& make clean \
	&& cmake --build . --config Release -- -j $(getconf _NPROCESSORS_ONLN) \
	&& apk del --purge .gserver-build-dependencies

# GServer Run Environment
FROM alpine:3.10
ARG CACHE_DATE=2016-01-01
COPY --from=build-env /gserver/bin /gserver
RUN apk add --update libstdc++ libatomic
WORKDIR /gserver
VOLUME /gserver/servers
ENTRYPOINT ["./gs2emu"]
CMD []
