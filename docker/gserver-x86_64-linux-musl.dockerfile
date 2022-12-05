ARG NPCSERVER=on
ARG VER_EXTRA=""

FROM xtjoeytx/v8:9.1.269.9 as v8

# GServer Build Environment
FROM alpine:3.16 AS build-env-npcserver-on
ONBUILD COPY --chown=1001:1001 --from=v8 /tmp/v8 /tmp/gserver/dependencies/v8

FROM alpine:3.16 AS build-env-npcserver-off

FROM build-env-npcserver-${NPCSERVER} AS build-env
ARG NPCSERVER
ARG VER_EXTRA
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
		automake \
		autoconf \
		ninja \
	&& cd /tmp/gserver \
	&& cmake -GNinja -S/tmp/gserver -B/tmp/gserver/build -DCMAKE_BUILD_TYPE=Release -DV8NPCSERVER=${NPCSERVER} -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=OFF -DUPNP=OFF -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
	&& cmake --build /tmp/gserver/build --config Release --target clean \
	&& cmake --build /tmp/gserver/build --config Release --target package --parallel $(getconf _NPROCESSORS_ONLN) \
	&& chmod 777 -R /tmp/gserver/dist \
	&& rm -rf /tmp/gserver/dist/_CPack_Packages \
    && chown 1001:1001 -R /tmp/gserver \
    && apk del --purge .gserver-build-dependencies

USER 1001
# GServer Run Environment
FROM alpine:3.16
ARG CACHE_DATE=2021-07-25
COPY --from=build-env /tmp/gserver/bin /gserver
COPY --from=build-env /tmp/gserver/dist /dist
COPY entrypoint.sh /gserver/
RUN apk add --update libstdc++ libatomic
WORKDIR /gserver
VOLUME /gserver/servers
ENTRYPOINT ["/gserver/entrypoint.sh"]
CMD ["/gserver/gs2emu"]
