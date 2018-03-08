# git clone https://github.com/Linuxbrew/brew.git $LINUXBREWHOME;
# export HOMEBREW_FORCE_VENDOR_RUBY=1
# export PATH=$LINUXBREWHOME/bin:$PATH
# export MANPATH=$LINUXBREWHOME/share/man:$MANPATH
# export INFOPATH=$LINUXBREWHOME/share/info:$INFOPATH
# export PKG_CONFIG_PATH=$LINUXBREWHOME/lib64/pkgconfig:$LINUXBREWHOME/lib/pkgconfig:$PKG_CONFIG_PATH
# export LD_LIBRARY_PATH=$LINUXBREWHOME/lib64:$LINUXBREWHOME/lib:$LD_LIBRARY_PATH

if [ -z "$(ls -A /home/linuxbrew/.linuxbrew)" ]; then
    sh -c "$(curl -fsSL https://raw.githubusercontent.com/Linuxbrew/install/master/install.sh)";
fi
export HOMEBREW_BUILD_FROM_SOURCE=1
export PATH="/home/linuxbrew/.linuxbrew/bin:$PATH"
export MANPATH="/home/linuxbrew/.linuxbrew/share/man:$MANPATH"
export INFOPATH="/home/linuxbrew/.linuxbrew/share/info:$INFOPATH"
brew update
brew install msgpack
brew install fmt
brew install czmq -v
brew install boost -v
