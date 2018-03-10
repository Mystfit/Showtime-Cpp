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
brew install gcc@6

export HOMEBREW_BUILD_FROM_SOURCE=1 
export CFLAGS="-D_GLIBCXX_USE_CXX11_ABI=1"
export HOMEBREW_CC=gcc-6
brew install msgpack
brew install fmt
brew install czmq --with-drafts
brew install boost --without-single --without-static
