ARG NPCSERVER=on
ARG VER_EXTRA=""

FROM xtjoeytx/v8:9.1.269.9-mingw as v8

# GServer Build Environment
FROM amigadev/crosstools:x86_64-w64-mingw32 AS build-env
ARG NPCSERVER
ARG VER_EXTRA
COPY --from=v8 /tmp/v8 /usr/x86_64-w64-mingw32/usr
RUN ln -s /usr/x86_64-w64-mingw32/include/wincrypt.h /usr/x86_64-w64-mingw32/include/Wincrypt.h

USER 1001
COPY --chown=1001:1001 ./ /tmp/gserver

RUN cd /tmp/gserver \
	&& cmake -GNinja -S/tmp/gserver -B/tmp/gserver/build -DCMAKE_BUILD_TYPE=Release -DSTATIC=ON -DV8NPCSERVER=${NPCSERVER} -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=ON -DUPNP=OFF -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
	&& cmake --build /tmp/gserver/build --target clean \
	&& cmake --build /tmp/gserver/build --target package --parallel $(getconf _NPROCESSORS_ONLN) \
	&& chmod 777 -R /tmp/gserver/dist \
    && rm -rf /tmp/gserver/dist/_CPack_Packages

# GServer Run Environment
FROM alpine:3.14
ARG CACHE_DATE=2021-07-25
COPY --from=build-env /tmp/gserver/dist /dist
USER 1001
WORKDIR /gserver

