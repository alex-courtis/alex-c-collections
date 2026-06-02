#!/bin/sh

docker run --rm -ti \
	--volume "${PWD}:/alex-c-collections" \
	--workdir="/alex-c-collections" \
	--user "`id -u`:`id -g`" \
	alex-c-collections:latest \
	$@
