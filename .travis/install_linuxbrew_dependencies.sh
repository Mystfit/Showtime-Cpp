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
brew install gcc@6
brew postinstall glibc
export CFLAGS="-D_GLIBCXX_USE_CXX11_ABI=1"
export HOMEBREW_CC=gcc-6

# Zeromq, boost, msgpack unbundled dependencies
brew install autoconf, pkg-config, gpatch, ncurses, readline, sqlite, gdbm, \
	berkeley-db, libbsd, expat, perl, openssl, bzip2, python@2, libxml2, 	\
	docbook, docbook-xsl, gettext, gnu-getopt, libxslt, xmlto, xz, libffi,  \
	python, asciidoc, zlib, cmake
/

# Bottled dependencies
brew install msgpack
brew install fmt

# Source dependencies
export HOMEBREW_BUILD_FROM_SOURCE=1 
brew install zeromq --with-drafts
brew install czmq --with-drafts
brew install boost --without-single --without-static
