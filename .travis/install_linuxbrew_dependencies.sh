# git clone https://github.com/Linuxbrew/brew.git $LINUXBREWHOME;
# export HOMEBREW_FORCE_VENDOR_RUBY=1
# export PATH=$LINUXBREWHOME/bin:$PATH
# export MANPATH=$LINUXBREWHOME/share/man:$MANPATH
# export INFOPATH=$LINUXBREWHOME/share/info:$INFOPATH
# export PKG_CONFIG_PATH=$LINUXBREWHOME/lib64/pkgconfig:$LINUXBREWHOME/lib/pkgconfig:$PKG_CONFIG_PATH
# export LD_LIBRARY_PATH=$LINUXBREWHOME/lib64:$LINUXBREWHOME/lib:$LD_LIBRARY_PATH
echo 'export PATH="/home/linuxbrew/.linuxbrew/bin:$PATH"' >>~/.profile
echo 'export MANPATH="/home/linuxbrew/.linuxbrew/share/man:$MANPATH"' >>~/.profile
echo 'export INFOPATH="/home/linuxbrew/.linuxbrew/share/info:$INFOPATH"' >>~/.profile
PATH="/home/linuxbrew/.linuxbrew/bin:$PATH"
brew install msgpack
brew install fmt
brew install czmq -v
brew install boost -v
