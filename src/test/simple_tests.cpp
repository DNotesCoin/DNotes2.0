#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(simple_tests)

BOOST_AUTO_TEST_CASE(test1)
{
        BOOST_CHECK(true == true);
}


BOOST_AUTO_TEST_CASE(test2)
{
        //BOOST_CHECK(true == false);
}


BOOST_AUTO_TEST_SUITE_END()
