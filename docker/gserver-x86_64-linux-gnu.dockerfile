ARG NPCSERVER=on
ARG VER_EXTRA=""

FROM xtjoeytx/v8:9.1.269.9-gnu as v8

# GServer Build Environment
FROM amigadev/crosstools:x86_64-linux AS build-env-npcserver-on
ONBUILD COPY --chown=1001:1001 --from=v8 /tmp/v8 /tmp/v8

FROM amigadev/crosstools:x86_64-linux AS build-env-npcserver-off

FROM build-env-npcserver-${NPCSERVER} AS build-env
ARG NPCSERVER
ARG VER_EXTRA
USER 1001
COPY --chown=1001:1001 ./ /tmp/gserver

RUN cd /tmp/gserver \
    && ln -s /tmp/v8 /tmp/gserver/dependencies/v8 \
	&& cmake -GNinja -S/tmp/gserver -B/tmp/gserver/build -DCMAKE_BUILD_TYPE=Release -DSTATIC=ON -DV8NPCSERVER=${NPCSERVER} -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=OFF -DUPNP=OFF -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
	&& cmake --build /tmp/gserver/build --target clean \
	&& cmake --build /tmp/gserver/build --target package --parallel $(getconf _NPROCESSORS_ONLN) \
	&& chmod 777 -R /tmp/gserver/dist \
    && rm -rf /tmp/gserver/dist/_CPack_Packages

# GServer Run Environment
FROM alpine:3.14
ARG CACHE_DATE=2021-07-25
COPY --from=build-env /tmp/gserver/dist /dist
COPY --from=build-env /tmp/gserver/build /build
RUN apk add --update libstdc++ libatomic cmake
USER 1001
WORKDIR /gserver

