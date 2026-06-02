#
# Install packages within the docker container.
# Not for running locally.
#

# 
# arch 64 and 32 bit packages
#
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
