#define BOOST_TEST_MODULE URI

#include "TestCommon.hpp"

using namespace ZstTest;

ZstURI uri_empty = ZstURI();
ZstURI uri_single = ZstURI("single");
ZstURI uri_equal1 = ZstURI("ins/someplug");
ZstURI uri_notequal = ZstURI("anotherins/someplug");

BOOST_AUTO_TEST_CASE(standard_layout){
	assert(std::is_standard_layout<ZstURI>());
}

BOOST_AUTO_TEST_CASE(accessors) {
	BOOST_TEST(strcmp(uri_equal1.segment(0), "ins") == 0);
	BOOST_TEST(strcmp(uri_equal1.segment(1), "someplug") == 0);
	BOOST_TEST(uri_empty.is_empty());
	BOOST_TEST(uri_equal1.size() == 2);
	BOOST_TEST(strcmp(uri_equal1.path(), "ins/someplug") == 0);
}

BOOST_AUTO_TEST_CASE(comparisons) {
	auto a = ZstURI("a");
	auto b = ZstURI("b");
	auto c = ZstURI("c");

	BOOST_TEST(ZstURI::equal(uri_equal1, uri_equal1));
	BOOST_TEST(!ZstURI::equal(uri_equal1, uri_notequal));
	BOOST_TEST(ZstURI::equal(ZstURI("inplace"), ZstURI("inplace")));
	BOOST_TEST(b < c);
	BOOST_TEST(a < b);
	BOOST_TEST(a < c);
	BOOST_TEST(!(c < a));
	BOOST_TEST(uri_equal1.contains(ZstURI("ins")));
	BOOST_TEST(!ZstURI("ins").contains(uri_equal1));
	BOOST_TEST(uri_equal1.contains(ZstURI("ins/someplug")));
	BOOST_TEST(!uri_equal1.contains(ZstURI("nomatch")));
}

BOOST_AUTO_TEST_CASE(ranges) {
	BOOST_TEST(ZstURI::equal(uri_equal1.range(0, 1), ZstURI("ins/someplug")));
	BOOST_TEST(ZstURI::equal(uri_equal1.range(1, 1), ZstURI("someplug")));
	BOOST_TEST(ZstURI::equal(uri_equal1.range(0, 0), ZstURI("ins")));
}

BOOST_AUTO_TEST_CASE(parents) {
	auto parent = ZstURI("ins");
	BOOST_TEST(ZstURI::equal(uri_equal1.parent(), parent));
	BOOST_TEST(ZstURI::equal(uri_equal1.first(), parent));
	BOOST_CHECK_THROW(uri_single.parent(), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(joins) {
	ZstURI joint_uri = ZstURI("a") + ZstURI("b");
	BOOST_TEST(ZstURI::equal(joint_uri, ZstURI("a/b")));
	BOOST_TEST(joint_uri.size() == 2);
	BOOST_TEST(joint_uri.full_size() == 3);
}

BOOST_AUTO_TEST_CASE(streaming) {
	std::stringstream s;
	s << uri_single;
	BOOST_TEST(strcmp(s.str().c_str(), uri_single.path()) == 0);
}

BOOST_AUTO_TEST_CASE(scope) {
	{
		ZstURI stack_uri("some_entity/some_name");
	}
}

BOOST_AUTO_TEST_CASE(range_exceptions) {
	BOOST_CHECK_THROW(uri_equal1.segment(2), std::range_error);
	BOOST_CHECK_THROW(uri_equal1.range(0, 4), std::range_error);
}
