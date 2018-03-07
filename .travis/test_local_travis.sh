TRAVIS_BUILD_DIR=/mnt/c/projects/showtime-cpp-travis
TRAVIS_OS_NAME="linux"
BUILD_FOLDER=$TRAVIS_BUILD_DIR
GENERATOR="Unix Makefiles"
USE_LOCAL_BOOST=1
BOOST_ROOT=$TRAVIS_BUILD_DIR/boost
LINUXBREWHOME=$BUILD_FOLDER/linuxbrew

cd /mnt/c/projects
git clone https://github.com/mystfit/showtime-cpp.git showtime-cpp-travis
cd showtime-cpp-travis
git checkout travis

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    sudo apt-get -qq update;
    sudo apt-get install build-essential curl file git python-setuptools;
fi

BOOST_EXISTS=1
if [ -z "$(ls -A $BOOST_ROOT)" ]; then BOOST_EXISTS=0; fi
echo "Boost exists: $BOOST_EXISTS"
if [ "$TRAVIS_OS_NAME" == "linux" ] && [ $BOOST_EXISTS -eq 0 ]; then 
    mkdir $BOOST_ROOT;
    BOOST_FLAGS="--with-atomic";
    BOOST_FLAGS="$BOOST_FLAGS --with-chrono";
    BOOST_FLAGS="$BOOST_FLAGS --with-date_time";
    BOOST_FLAGS="$BOOST_FLAGS --with-filesystem";
    BOOST_FLAGS="$BOOST_FLAGS --with-log";
    BOOST_FLAGS="$BOOST_FLAGS --with-system";
    BOOST_FLAGS="$BOOST_FLAGS --with-thread";
    wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2;
    tar --bzip2 -xf boost_1_66_0.tar.bz2;
    pushd boost_1_66_0;
    ./bootstrap.sh --prefix=$BOOST_ROOT;
fi
if [ $BOOST_EXISTS -eq 0 ]; then 
    ./b2 --prefix=$BOOST_ROOT --toolset=gcc $BOOST_FLAGS link=static stage;
    ./b2 --prefix=$BOOST_ROOT --toolset=gcc $BOOST_FLAGS link=static install;
    popd;
fi

LINUXBREW_EXISTS=1
if [ -z "$(ls -A $LINUXBREWHOME)" ]; then LINUXBREW_EXISTS=0; fi
echo "Linuxbrew exists: $LINUXBREW_EXISTS"
if [ "$TRAVIS_OS_NAME" == "linux" ] && [ $LINUXBREW_EXISTS -eq 0 ]; then 
    git clone https://github.com/Linuxbrew/brew.git $LINUXBREWHOME;
    export HOMEBREW_FORCE_VENDOR_RUBY=1;
    export PATH=$LINUXBREWHOME/bin:$PATH;
    export MANPATH=$LINUXBREWHOME/share/man:$MANPATH;
    export INFOPATH=$LINUXBREWHOME/share/info:$INFOPATH;
    export PKG_CONFIG_PATH=$LINUXBREWHOME/lib64/pkgconfig:$LINUXBREWHOME/lib/pkgconfig:$PKG_CONFIG_PATH;
    export LD_LIBRARY_PATH=$LINUXBREWHOME/lib64:$LINUXBREWHOME/lib:$LD_LIBRARY_PATH;
fi

if [ $LINUXBREW_EXISTS -eq 0 ]; then brew install czmq; fi
if [ $LINUXBREW_EXISTS -eq 0 ]; then brew install msgpack; fi
if [ $LINUXBREW_EXISTS -eq 0 ]; then brew install fmt; fi
