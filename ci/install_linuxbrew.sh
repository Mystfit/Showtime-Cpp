if [ ! -d "$HOME/.linuxbrew" ]; then
       	echo "Installing linuxbrew";
	git clone https://github.com/Linuxbrew/brew.git ~/.linuxbrew;
fi
export HOMEBREW_FORCE_VENDOR_RUBY=1;
PATH="$HOME/.linuxbrew/bin:$PATH";
export MANPATH="$(brew --prefix)/share/man:$MANPATH";
export INFOPATH="$(brew --prefix)/share/info:$INFOPATH";
echo "Set linuxbrew paths"

