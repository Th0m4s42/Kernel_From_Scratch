# ---------- Stage 1 : construction du cross-compiler ----------
FROM debian:bookworm-slim AS toolchain

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential wget ca-certificates bison flex libgmp-dev libmpc-dev \
    libmpfr-dev texinfo xz-utils \
    && rm -rf /var/lib/apt/lists/*

ENV PREFIX=/opt/cross
ENV TARGET=i686-elf
ENV PATH="${PREFIX}/bin:${PATH}"

WORKDIR /tmp/build

RUN wget -q https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz \
    && tar xf binutils-2.42.tar.xz \
    && mkdir build-binutils && cd build-binutils \
    && ../binutils-2.42/configure --target=$TARGET --prefix=$PREFIX \
       --with-sysroot --disable-nls --disable-werror \
    && make -j$(nproc) && make install

RUN wget -q https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz \
    && tar xf gcc-13.2.0.tar.xz \
    && mkdir build-gcc && cd build-gcc \
    && ../gcc-13.2.0/configure --target=$TARGET --prefix=$PREFIX \
       --disable-nls --enable-languages=c --without-headers \
    && make -j$(nproc) all-gcc all-target-libgcc \
    && make install-gcc install-target-libgcc

# ---------- Stage 2 : image finale ----------
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    nasm grub-pc-bin grub-common xorriso mtools make \
    libgmp10 libmpc3 libmpfr6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=toolchain /opt/cross /opt/cross
ENV PATH="/opt/cross/bin:${PATH}"

WORKDIR /os