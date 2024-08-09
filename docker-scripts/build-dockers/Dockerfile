################################################################################
###  Build cmake                                                             ###
################################################################################
FROM ubuntu:jammy as base_builder
RUN apt update && apt install -y lsb-release g++ build-essential wget git libssl-dev pkg-config
RUN wget https://github.com/Kitware/CMake/releases/download/v3.30.2/cmake-3.30.2.tar.gz
RUN tar -xvf cmake-3.30.2.tar.gz
RUN cd cmake-3.30.2 && ./bootstrap && make -j && make install
RUN useradd builder -m
RUN mkdir -p /home/builder/cc3dsfs
RUN mkdir -p /home/builder/building
RUN chown -R builder:builder /home/builder

################################################################################
###  Build x86_32                                                            ###
################################################################################
FROM base_builder as builder32
COPY linux_install_packages.sh /
RUN chmod +x /linux_install_packages.sh ; /linux_install_packages.sh i386
WORKDIR /home/builder/building
COPY  docker_build_starter /
RUN chmod +x /docker_build_starter
USER builder
CMD /docker_build_starter docker-scripts/docker_build_x86_32

################################################################################
###  Build x86_64                                                            ###
################################################################################
FROM base_builder as builder64
COPY linux_install_packages.sh /
RUN chmod +x /linux_install_packages.sh ; /linux_install_packages.sh amd64
WORKDIR /home/builder/building
COPY  docker_build_starter /
RUN chmod +x /docker_build_starter
USER builder
CMD /docker_build_starter docker-scripts/docker_build_x86_64

################################################################################
###  Build arm 32                                                            ###
################################################################################
FROM base_builder as builderarm32
COPY linux_install_packages.sh /
RUN chmod +x /linux_install_packages.sh ; /linux_install_packages.sh armhf
WORKDIR /home/builder/building
COPY  docker_build_starter /
RUN chmod +x /docker_build_starter
USER builder
CMD /docker_build_starter docker-scripts/docker_build_arm32

################################################################################
###  Build arm 64                                                            ###
################################################################################
FROM base_builder as builderarm64
COPY linux_install_packages.sh /
RUN chmod +x /linux_install_packages.sh ; /linux_install_packages.sh arm64
WORKDIR /home/builder/building
COPY  docker_build_starter /
RUN chmod +x /docker_build_starter
USER builder
CMD /docker_build_starter docker-scripts/docker_build_arm64
