# Set linuxbrew paths
export PATH="$HOMEBREW_PATH/bin:$PATH"
export MANPATH="$HOMEBREW_PATH/share/man:$MANPATH"
export INFOPATH="$HOMEBREW_PATH/share/info:$INFOPATH"

# Install linuxbrew if it doesn't already exist
if [ -z "$(ls -A $HOMEBREW_PATH)" ]; then
    sh -c "$(curl -fsSL https://raw.githubusercontent.com/Linuxbrew/install/master/install.sh)";
fi
brew update >/dev/null

# Locale fix to let glibc install properly
sudo sh -c 'echo en_US.UTF-8 UTF-8 >>/etc/locale.gen' && sudo /usr/sbin/locale-gen;
LANG=C LC_ALL=LC_ALL=en_US.UTF-8 LC_CTYPE=en_US.UTF-8 brew install glibc

# Install GCC and set environment vars
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
