if [ -z "$(ls -A $HOME/.linuxbrew)"  ]; then
   	echo "Installing linuxbrew to $HOME/.linuxbrew";
	git clone https://github.com/Linuxbrew/brew.git $HOME/.linuxbrew;
else
	echo "Found existing linuxbrew directory";
fi
export HOMEBREW_FORCE_VENDOR_RUBY=1
export PATH="$HOME/.linuxbrew/bin:$PATH"
export MANPATH="$(brew --prefix)/share/man:$MANPATH"
export INFOPATH="$(brew --prefix)/share/info:$INFOPATH"
echo "Set linuxbrew paths"
