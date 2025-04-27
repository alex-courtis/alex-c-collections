#!/bin/sh

pacman --noconfirm -Syu \
	clang \
	cmake \
	cmocka \
	cppcheck \
	git \
	lib32-cmocka \
	llvm \
	llvm-libs \
	ninja \
	valgrind
