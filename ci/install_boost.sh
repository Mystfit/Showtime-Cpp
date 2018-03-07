# sudo apt-get install libboost1.65-dev
# sudo apt-get install libboostsystem1.65-dev
# sudo apt-get install libboostfilesystem1.65-dev
# sudo apt-get install libboostlog1.65-dev
# sudo apt-get install libboostthread1.65-dev
# sudo apt-get install libboostdate_time1.65-dev
# sudo apt-get install libboostchrono1.65-dev
# sudo apt-get install libboostatomic1.65-dev

BOOST_FLAGS="--with-atomic"
BOOST_FLAGS="$BOOST_FLAGS --with-chrono"
BOOST_FLAGS="$BOOST_FLAGS --with-date_time"
BOOST_FLAGS="$BOOST_FLAGS --with-filesystem"
BOOST_FLAGS="$BOOST_FLAGS --with-log"
BOOST_FLAGS="$BOOST_FLAGS --with-system"
BOOST_FLAGS="$BOOST_FLAGS --with-thread"

wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2;
tar --bzip2 -xf boost_1_66_0.tar.bz2;
pushd boost_1_66_0;
./bootstrap.sh --prefix=$BOOST_ROOT
./b2 --prefix=$BOOST_ROOT --toolset=gcc $BOOST_FLAGS link=static stage -d0
./b2 --prefix=$BOOST_ROOT --toolset=gcc $BOOST_FLAGS link=static install -d0
popd
