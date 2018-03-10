# git clone https://github.com/Linuxbrew/brew.git $LINUXBREWHOME;
# export HOMEBREW_FORCE_VENDOR_RUBY=1
# export PATH=$LINUXBREWHOME/bin:$PATH
# export MANPATH=$LINUXBREWHOME/share/man:$MANPATH
# export INFOPATH=$LINUXBREWHOME/share/info:$INFOPATH
# export PKG_CONFIG_PATH=$LINUXBREWHOME/lib64/pkgconfig:$LINUXBREWHOME/lib/pkgconfig:$PKG_CONFIG_PATH
# export LD_LIBRARY_PATH=$LINUXBREWHOME/lib64:$LINUXBREWHOME/lib:$LD_LIBRARY_PATH

if [ -z "$(ls -A $HOMEBREW_PATH)" ]; then
    sh -c "$(curl -fsSL https://raw.githubusercontent.com/Linuxbrew/install/master/install.sh)";
fi
export PATH="$HOMEBREW_PATH/bin:$PATH"
export MANPATH="$HOMEBREW_PATH/share/man:$MANPATH"
export INFOPATH="$HOMEBREW_PATH/share/info:$INFOPATH"
brew update >/dev/null

# Install GCC and environment vars
brew install gcc
brew link gcc
which gcc
gcc --version
brew postinstall glibc
brew install binutils
which ld
ld --version
export CFLAGS="-D_GLIBCXX_USE_CXX11_ABI=1"

# Install unbundled dependencies to skip source compiliation
brew install autoconf
brew install pkg-config
brew install gpatch
brew install ncurses 
brew install readline
brew install sqlite
brew install gdbm
brew install berkeley-db
brew install libbsd
brew install expat
brew install perl
brew install openssl
brew install bzip2
brew install python@2
brew install libxml2
brew install docbook
brew install docbook-xsl
brew install gettext
brew install gnu-getopt
brew install libxslt
brew install xmlto
brew install xz
brew install libffi
brew install python
brew install asciidoc
brew install zlib
brew install cmake

# Bottled dependencies
brew install msgpack
brew install fmt

# Source dependencies
export HOMEBREW_BUILD_FROM_SOURCE=1 
brew install zeromq
brew install czmq
brew install boost --without-single --without-static
