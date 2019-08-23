FROM alpine:3.10 AS build-env

RUN apk update && apk upgrade
RUN apk add cmake gcc g++ bash make python git automake autoconf

COPY ./ /gserver

WORKDIR /gserver

RUN mkdir /gserver/build

WORKDIR /gserver/build

ENV PATH="/gserver/dependencies/depot_tools:${PATH}"

RUN cmake .. -DV8NPCSERVER=OFF -DNOUPNP=ON
RUN cmake --build . --config Debug -- -j8

WORKDIR /gserver/bin

RUN apk del --purge cmake gcc g++ bash make

FROM alpine:3.10 AS run-env
COPY --from=build-env /gserver/bin /gserver
RUN apk update && apk add libstdc++ libatomic

WORKDIR /gserver

ENTRYPOINT ./gs2emu

EXPOSE 14802
