ARG NPCSERVER=on
ARG VER_EXTRA=""

# GServer Build Environment
FROM amigadev/crosstools:x86_64-w64-mingw32 AS build-env
ARG NPCSERVER
ARG VER_EXTRA
USER root

RUN cd /tmp \
        && wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-v8-9.1.269.39-5-any.pkg.tar.zst -O /tmp/mingw-w64-v8.tar.zst \
        && wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-icu-72.1-1-any.pkg.tar.zst -O /tmp/mingw-w64-icu.tar.zst \
        && wget https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-zlib-1.2.13-2-any.pkg.tar.zst -O /tmp/mingw-w64-zlib.tar.zst \
        && wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-gcc-libs-12.2.0-6-any.pkg.tar.zst -O /tmp/mingw-w64-gcc-libs.tar.zst \
        && cd /usr/x86_64-w64-mingw32 \
        && tar xvf /tmp/mingw-w64-v8.tar.zst \
        && tar xvf /tmp/mingw-w64-icu.tar.zst \
        && tar xvf /tmp/mingw-w64-zlib.tar.zst \
        && tar xvf /tmp/mingw-w64-gcc-libs.tar.zst \
        && mkdir -p usr \
        && cp -fvr mingw64/* usr/

USER 1001
COPY --chown=1001:1001 ./ /tmp/gserver

RUN cd /tmp/gserver \
        && cmake -GNinja -S/tmp/gserver -B/tmp/gserver/build -DCMAKE_BUILD_TYPE=Release -DSTATIC=ON -DV8NPCSERVER=${NPCSERVER} -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=OFF -DUPNP=OFF -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
		&& cmake --build /tmp/gserver/build --target clean \
		&& cmake --build /tmp/gserver/build --target package --parallel $(getconf _NPROCESSORS_ONLN) \
    	&& chmod 777 -R /tmp/gserver/build

# GServer Run Environment
FROM alpine:3.14
ARG CACHE_DATE=2021-07-25
COPY --from=build-env /tmp/gserver/build /gserver
USER 1001
WORKDIR /gserver

