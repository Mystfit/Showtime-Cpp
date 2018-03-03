# sudo apt-get install libboost1.65-dev
# sudo apt-get install libboostsystem1.65-dev
# sudo apt-get install libboostfilesystem1.65-dev
# sudo apt-get install libboostlog1.65-dev
# sudo apt-get install libboostthread1.65-dev
# sudo apt-get install libboostdate_time1.65-dev
# sudo apt-get install libboostchrono1.65-dev
# sudo apt-get install libboostatomic1.65-dev

B2_CMD=./b2 --prefix=$BOOST_ROOT link=static
ALL_LIB_FLAGS=--with-thread --with-system --with-filesystem --with-log --with-date_time --with-chrono --with-atomic

wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2;
tar --bzip2 -xf boost_1_66_0.tar.bz2;
pushd boost_1_66_0;

./bootstrap.sh --prefix=$BOOST_ROOT
$B2_CMD --with-thread stage
$B2_CMD --with-system stage
$B2_CMD --with-filesystem stage
$B2_CMD --with-log stage
$B2_CMD --with-date_time stage
$B2_CMD --with-chrono stage
$B2_CMD --with-atomic stage
$B2_CMD $ALL_LIB_FLAGS install
popd