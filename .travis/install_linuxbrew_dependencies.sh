# Set linuxbrew paths
# export PATH="$HOMEBREW_PATH/bin:$HOMEBREW_PATH/sbin:$PATH"
# export MANPATH="$HOMEBREW_PATH/share/man:$MANPATH"
# export INFOPATH="$HOMEBREW_PATH/share/info:$INFOPATH"

# Install linuxbrew if it doesn't already exist
if [ -z "$(ls -A $HOMEBREW_PATH)" ]; then
    sh -c "$(curl -fsSL https://raw.githubusercontent.com/Linuxbrew/install/master/install.sh)";
fi
brew update >/dev/null
brew doctor

# Install GCC and set environment vars
brew install glibc
brew install gcc
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

# Install bottled dependencies
brew install msgpack
brew install fmt

# Install source dependencies
export HOMEBREW_BUILD_FROM_SOURCE=1 
brew install zeromq
brew install czmq
brew install boost --without-single
