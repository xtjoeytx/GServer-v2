# Google V8 Build Environment
FROM amigadev/crosstools:x86_64-linux as v8
COPY dependencies/build-v8-linux /tmp/

RUN cd /tmp/ && \
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git /tmp/depot_tools && \
    ln -s /usr/bin/python2.7 /tmp/depot_tools/python && \
	./build-v8-linux

FROM alpine:3.16 as v8-final
COPY --from=v8 /tmp/v8 /tmp/v8
