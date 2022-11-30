ARG NPCSERVER=on
ARG VER_EXTRA=""

# GServer Build Environment
FROM amigadev/crosstools:x86_64-w64-mingw32 AS build-env
COPY ./ /gserver
ARG NPCSERVER
ARG VER_EXTRA

RUN cd /gserver \
        && wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-v8-9.1.269.39-5-any.pkg.tar.zst -O /gserver/mingw-w64-v8.tar.zst \
        && wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-icu-72.1-1-any.pkg.tar.zst -O /gserver/mingw-w64-icu.tar.zst \
        && wget https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-zlib-1.2.13-2-any.pkg.tar.zst -O /gserver/mingw-w64-zlib.tar.zst \
        && wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-gcc-libs-12.2.0-6-any.pkg.tar.zst -O /gserver/mingw-w64-gcc-libs.tar.zst \
        && cd /usr/x86_64-w64-mingw32 \
        && tar xvf /gserver/mingw-w64-v8.tar.zst \
        && tar xvf /gserver/mingw-w64-icu.tar.zst \
        && tar xvf /gserver/mingw-w64-zlib.tar.zst \
        && tar xvf /gserver/mingw-w64-gcc-libs.tar.zst \
        && mkdir -p usr \
        && cp -fvr mingw64/* usr/ \
        && cd /gserver \
        && cmake -GNinja -S/gserver -B/gserver/build -DCMAKE_BUILD_TYPE=Release -DSTATIC=ON -DV8NPCSERVER=${NPCSERVER} -DVER_EXTRA=${VER_EXTRA} -DWOLFSSL=OFF -DUPNP=OFF -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffast-math" \
		&& cmake --build /gserver/build --target clean \
		&& cmake --build /gserver/build --target package --parallel $(getconf _NPROCESSORS_ONLN)

# GServer Run Environment
FROM alpine:3.14
ARG CACHE_DATE=2021-07-25
COPY --from=build-env /gserver/build /gserver
WORKDIR /gserver

