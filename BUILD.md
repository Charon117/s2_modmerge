## Standard build

init git submodules: 'git submodule update --init'

While configuratating cmake_options you might want to choose your compiler and flags:

-DCMAKE_CXX_COMPILER=<compiler/path/here>

And for gcc/MINGW: -DCMAKE_EXE_LINKER_FLAGS="-static -libstdc++"

For verbose output: -DCMAKE_VERBOSE_MAKEFILE="ON"

Otherwise: 'cmake <cmake_options> ..'

then start the build process
