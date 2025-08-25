ARG CMAKE_VERSION_SYSTEM="3.30.9"
ARG CMAKE_VERSION_SDKMANAGER_ANDROID="3.30.5"

################################################################################
###  Build cmake                                                             ###
################################################################################
FROM ubuntu:jammy as cmake_builder
ARG CMAKE_VERSION_SYSTEM
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Paris
RUN apt update && apt install -y lsb-release g++ build-essential wget git libssl-dev pkg-config
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION_SYSTEM}/cmake-${CMAKE_VERSION_SYSTEM}.tar.gz
RUN tar -xvf cmake-${CMAKE_VERSION_SYSTEM}.tar.gz
RUN cd cmake-${CMAKE_VERSION_SYSTEM} && ./bootstrap && make -j && make install && cpack -G TGZ -C Release && mv cmake-${CMAKE_VERSION_SYSTEM}-Linux-x86_64.tar.gz ../cmake.tar.gz

################################################################################
###  Build cmake (focal)                                                     ###
################################################################################
FROM ubuntu:focal as cmake_builder_focal
ARG CMAKE_VERSION_SYSTEM
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Paris
RUN apt update && apt install -y lsb-release g++ build-essential wget git libssl-dev pkg-config
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION_SYSTEM}/cmake-${CMAKE_VERSION_SYSTEM}.tar.gz
RUN tar -xvf cmake-${CMAKE_VERSION_SYSTEM}.tar.gz
RUN cd cmake-${CMAKE_VERSION_SYSTEM} && ./bootstrap && make -j && make install && cpack -G TGZ -C Release && mv cmake-${CMAKE_VERSION_SYSTEM}-Linux-x86_64.tar.gz ../cmake.tar.gz

################################################################################
###  Prepare generic builder                                                 ###
################################################################################
FROM ubuntu:jammy as base_builder
RUN apt update && apt install -y lsb-release g++ build-essential wget git libssl-dev pkg-config
COPY --from=cmake_builder cmake.tar.gz ./cmake.tar.gz
RUN mkdir -p cmake_folder && cd cmake_folder && tar -xvf ../cmake.tar.gz --strip-components=1 && cp -rf * /usr/local/ && cd ..
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

################################################################################
###  Build Android                                                           ###
################################################################################
# Different from all others
FROM saschpe/android-ndk:31-jdk11.0.13_8-ndk23.0.7599858-cmake3.18.1 as builderandroid
ARG CMAKE_VERSION_SDKMANAGER_ANDROID
RUN apt update && apt install -y lsb-release g++ build-essential wget git libssl-dev pkg-config zip unzip
RUN curl -s "https://get.sdkman.io" | bash
RUN /bin/bash -c "source /root/.sdkman/bin/sdkman-init.sh ; sdk install gradle 7.6.2"
RUN sdkmanager "build-tools;30.0.3" "cmake;${CMAKE_VERSION_SDKMANAGER_ANDROID}"
ENV ANDROID_SDK_VERSION="31"
ENV ANDROID_HOME="/opt/android-sdk-linux"
ENV ANDROID_NDK_VERSION="23.0.7599858"
ENV ANDROID_NDK_HOME="$ANDROID_HOME/ndk/$ANDROID_NDK_VERSION"
ENV GRADLE_DIR="/home/builder/gradle"
ENV GRADLE_HOME="$GRADLE_DIR/current"
ENV GRADLE_USER_HOME=$GRADLE_HOME
ENV PATH="$ANDROID_HOME:$GRADLE_HOME/bin:$ANDROID_NDK_HOME:$PATH"
RUN useradd builder -m
RUN mkdir -p /home/builder/cc3dsfs
RUN mkdir -p /home/builder/building
RUN chown -R builder:builder /home/builder
WORKDIR /home/builder/building
COPY  docker_build_starter /
RUN chmod +x /docker_build_starter
RUN chown -R builder:builder $ANDROID_HOME
RUN mkdir -p $GRADLE_DIR
RUN mv /root/.sdkman/candidates/gradle/* $GRADLE_DIR/
RUN chown -R builder:builder $GRADLE_DIR
COPY --from=cmake_builder_focal cmake.tar.gz /cmake.tar.gz
RUN mkdir -p /cmake_folder && cd /cmake_folder && tar -xvf /cmake.tar.gz --strip-components=1 && cp -rf * /usr/local/
USER builder
CMD /docker_build_starter docker-scripts/docker_build_android
