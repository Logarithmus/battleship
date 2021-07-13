FROM alpine:edge

RUN echo "Updating Alpine"
RUN apk update && apk upgrade

RUN echo "Installing dependencies..."
RUN apk add \
	cmake \
	make \
	gcc \
	g++ \
	openssl-dev \
	boost-dev \
	bzip2-dev \
	zlib-dev \
	clang-extra-tools \
	cppcheck \
