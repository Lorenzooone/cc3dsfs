#!/bin/bash

for image in "builder32" "builder64" "builderarm32" "builderarm64" "builderriscv64" "builderandroid" ;
do
	docker image push lorenzooone/cc3dsfs:${image}
done
