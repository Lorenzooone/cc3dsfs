#!/bin/bash
docker image rm lorenzooone/cc3dsfs:builder32
docker build --target builder32 . -t lorenzooone/cc3dsfs:builder32
docker image rm lorenzooone/cc3dsfs:builder64
docker build --target builder64 . -t lorenzooone/cc3dsfs:builder64
docker image rm lorenzooone/cc3dsfs:builderarm32
docker build --target builderarm32 . -t lorenzooone/cc3dsfs:builderarm32
docker image rm lorenzooone/cc3dsfs:builderarm64
docker build --target builderarm64 . -t lorenzooone/cc3dsfs:builderarm64
docker image rm lorenzooone/cc3dsfs:builderandroid
docker build --target builderandroid . -f Dockerfile_Android -t lorenzooone/cc3dsfs:builderandroid
