FROM arm64v8/ubuntu

SHELL ["/bin/bash", "-c"]

RUN apt-get -q update -y && \
    apt-get -q install -y --no-install-recommends \
    sudo \
    git \
    cmake \
    ninja-build \
    lsb-release wget software-properties-common gnupg \
    libstdc++6

RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 16 all && \
    rm -f llvm.sh; \
    ln -s /usr/bin/clang-16 /usr/bin/clang; \
    ln -s /usr/bin/clang++-16 /usr/bin/clang++; \
    ln -s /usr/bin/lldb-16 /usr/bin/lldb;

# Change these values by your uid/gid during build
ARG USERID=1000
ARG GROUPID=1000

# Create development user
ENV USER=me
RUN groupadd --gid ${GROUPID} --force me && \
    useradd --shell /bin/bash --home-dir /home/me --uid ${USERID} --gid ${GROUPID} --create-home me && \
    echo "me ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
USER me
WORKDIR /home/me