git clone https://github.com/Linuxbrew/brew.git $LINUXBREWHOME;
export HOMEBREW_FORCE_VENDOR_RUBY=1
export PATH=$LINUXBREWHOME/bin:$PATH
export MANPATH=$LINUXBREWHOME/share/man:$MANPATH
export INFOPATH=$LINUXBREWHOME/share/info:$INFOPATH
export PKG_CONFIG_PATH=$LINUXBREWHOME/lib64/pkgconfig:$LINUXBREWHOME/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=$LINUXBREWHOME/lib64:$LINUXBREWHOME/lib:$LD_LIBRARY_PATH

brew install patchelf
brew install zlib
brew install binutils
brew install linux-headers
travis_wait brew install glibc
brew install m4
brew install gmp
brew install mpfr
brew install libmpc
brew install isl
travis_wait brew install gcc
brew install isl
brew install zeromq
brew install czmq
brew install msgpack
brew install fmt
