# Google V8 Build Environment
FROM alpine:3.16 AS v8
USER root

RUN \
    	apk add --update --virtual .v8-build-dependencies \
    		tar \
    		zstd \
    		xz \
    	&& cd /tmp \
        && wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-v8-9.1.269.39-5-any.pkg.tar.zst -O /tmp/mingw-w64-v8.tar.zst \
        && wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-icu-72.1-1-any.pkg.tar.zst -O /tmp/mingw-w64-icu.tar.zst \
        && wget https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-zlib-1.2.13-2-any.pkg.tar.zst -O /tmp/mingw-w64-zlib.tar.zst \
        && wget https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-gcc-libs-12.2.0-6-any.pkg.tar.zst -O /tmp/mingw-w64-gcc-libs.tar.zst \
        && tar xvf /tmp/mingw-w64-v8.tar.zst \
        && tar xvf /tmp/mingw-w64-icu.tar.zst \
        && tar xvf /tmp/mingw-w64-zlib.tar.zst \
        && tar xvf /tmp/mingw-w64-gcc-libs.tar.zst \
        && mv -fv mingw64 v8 \
    	&& rm -rf *.zs \
    	&& apk del --purge .v8-build-dependencies
