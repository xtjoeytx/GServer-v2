ARG NPCSERVER=on
ARG VER_EXTRA=""

FROM xtjoeytx/v8:9.1.269.9 as v8

# GServer Build Environment
FROM alpine:3.14 AS build-env-npcserver-on
ONBUILD COPY --from=v8 /tmp/v8 /gserver/dependencies/v8

FROM alpine:3.14 AS build-env-npcserver-off

FROM build-env-npcserver-${NPCSERVER} AS build-env
COPY ./ /gserver
ARG NPCSERVER
ARG VER_EXTRA

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
	&& cmake -S/gserver -B/gserver/build -DCMAKE_BUILD_TYPE=Release -DV8NPCSERVER=${NPCSERVER} -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=OFF -DUPNP=OFF -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
	&& cmake --build /gserver/build --config Release --target clean \
	&& cmake --build /gserver/build --config Release -- -j $(getconf _NPROCESSORS_ONLN) \
	&& apk del --purge .gserver-build-dependencies

# GServer Run Environment
FROM alpine:3.14
ARG CACHE_DATE=2021-07-25
COPY --from=build-env /gserver/bin /gserver
COPY entrypoint.sh /gserver/
RUN apk add --update libstdc++ libatomic
WORKDIR /gserver
VOLUME /gserver/servers
ENTRYPOINT ["/gserver/entrypoint.sh"]
CMD ["/gserver/gs2emu"]
