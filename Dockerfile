FROM alpine:3.10 as gn-builder
ARG GN_COMMIT=d7111cb6877187d1f378bd231e14ffdd5fdd87ae
RUN apk add --update --virtual .gn-build-dependencies \
		alpine-sdk \
		binutils-gold \
		clang \
		curl \
		git \
		llvm \
		ninja \
		python \
		tar \
		xz \
	&& PATH=$PATH:/usr/lib/llvm4/bin \
	&& cp -f /usr/bin/ld.gold /usr/bin/ld \
	&& git clone https://gn.googlesource.com/gn /tmp/gn \
	&& git -C /tmp/gn checkout ${GN_COMMIT} \
	&& cd /tmp/gn \
	&& python build/gen.py --no-sysroot \
	&& ninja -C out \
	&& cp -f /tmp/gn/out/gn /usr/local/bin/gn \
	&& apk del .gn-build-dependencies \
	&& rm -rf /tmp/* /var/tmp/* /var/cache/apk/*

# Google V8 Clone Environment
# gclient does NOT work with Alpine
FROM debian:buster-slim as source
ARG V8_VERSION=7.4.288.26
RUN set -x && \
	apt-get update && \
	apt-get install -y \
		git \
		curl \
		python && \
	git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git /tmp/depot_tools && \
	PATH=$PATH:/tmp/depot_tools && \
	cd /tmp && \
	fetch v8 && \
	cd /tmp/v8 && \
	git checkout ${V8_VERSION} && \
	gclient sync -D && \
	apt-get remove --purge -y \
		git \
		curl \
		python && \
	apt-get autoremove -y && \
	rm -rf /var/lib/apt/lists/*

# Google V8 Build Environment
FROM alpine:3.10 as v8
COPY --from=source /tmp/v8 /tmp/v8
COPY --from=gn-builder /usr/local/bin/gn /tmp/v8/buildtools/linux64/gn
RUN \
	apk add --update --virtual .v8-build-dependencies \
		curl \
		g++ \
		gcc \
		glib-dev \
		icu-dev \
		libstdc++ \
		linux-headers \
		make \
		ninja \
		python \
		tar \
		xz \
	&& cd /tmp/v8 && \
	./tools/dev/v8gen.py x64.release -- \
		is_component_build=false \
		is_debug=false \
		use_custom_libcxx=false \
		v8_monolithic=true \
		v8_use_external_startup_data=false \
		binutils_path=\"/usr/bin\" \
		target_os=\"linux\" \
		target_cpu=\"x64\" \
		v8_target_cpu=\"x64\" \
		v8_enable_future=true \
		is_official_build=true \
		is_cfi=false \
		is_clang=false \
		use_custom_libcxx=false \
		use_sysroot=false \
		use_gold=false \
		use_allocator_shim=false \
		treat_warnings_as_errors=false \
		symbol_level=0 \
		strip_debug_info=true \
		v8_use_external_startup_data=false \
		v8_enable_i18n_support=false \
		v8_enable_gdbjit=false \
		v8_static_library=true \
		v8_experimental_extra_library_files=[] \
		v8_extra_library_files=[] \
	&& ninja -C out.gn/x64.release -j $(getconf _NPROCESSORS_ONLN) \
	&& find /tmp/v8/out.gn/x64.release -name '*.a' \
	&& apk del --purge .v8-build-dependencies

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
	&& cmake --build . --config Release -- -j $(getconf _NPROCESSORS_ONLN) \
	&& apk del --purge .gserver-build-dependencies

# GServer Run Environment
FROM alpine:3.10 AS run-env
COPY --from=build-env /gserver/bin /gserver
RUN apk add --update libstdc++ libatomic
WORKDIR /gserver
VOLUME /gserver/servers
ENTRYPOINT ./gs2emu
EXPOSE 14802
