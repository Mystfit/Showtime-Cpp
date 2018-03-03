# sudo apt-get install libboost1.65-dev
# sudo apt-get install libboostsystem1.65-dev
# sudo apt-get install libboostfilesystem1.65-dev
# sudo apt-get install libboostlog1.65-dev
# sudo apt-get install libboostthread1.65-dev
# sudo apt-get install libboostdate_time1.65-dev
# sudo apt-get install libboostchrono1.65-dev
# sudo apt-get install libboostatomic1.65-dev

B2_CMD=./b2 --prefix=$BOOST_ROOT --toolset=gcc link=static 
BOOST_LIBS=thread,system,filesystem,log,date_time,chrono,atomic

wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2;
tar --bzip2 -xf boost_1_66_0.tar.bz2;
pushd boost_1_66_0;

./bootstrap.sh --prefix=$BOOST_ROOT
$B2_CMD --with-libraries=$BOOST_LIBS stage
$B2_CMD --with-libraries=$BOOST_LIBS install
popd