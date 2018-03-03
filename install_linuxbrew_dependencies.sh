export LINUXBREWHOME="$TRAVIS_BUILD_DIR/.linuxbrew"
if [ -z "$(ls -A $LINUXBREWHOME)" ]; then 
  echo "Installing linuxbrew to $LINUXBREWHOME";
  git clone https://github.com/Linuxbrew/brew.git $LINUXBREWHOME;
else
  echo "Found existing linuxbrew directory";
fi
export HOMEBREW_FORCE_VENDOR_RUBY=1
export PATH=$LINUXBREWHOME/bin:$PATH
export MANPATH=$LINUXBREWHOME/share/man:$MANPATH
export INFOPATH=$LINUXBREWHOME/share/info:$INFOPATH
export PKG_CONFIG_PATH=$LINUXBREWHOME/lib64/pkgconfig:$LINUXBREWHOME/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=$LINUXBREWHOME/lib64:$LINUXBREWHOME/lib:$LD_LIBRARY_PATH
brew install czmq
brew install msgpack
brew install fmt