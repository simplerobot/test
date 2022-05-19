#include "Test.hpp"
#include <stdexcept>
#include <thread>


TEST_CASE(TestCaseListItem_Constructor_HappyCase)
{
	TestCaseListItem test([](){}, "TEST_CONSTRUCTOR", "FILE", 1234);
}

TEST_CASE(TestCaseListItem_Run_HappyCase)
{
	TestCaseListItem test([](){}, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(test.Run());
}

TEST_CASE(TestCaseListItem_Run_TestFails)
{
	TestCaseListItem test([](){ ASSERT(false); }, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(!test.Run());
}

TEST_CASE(ASSERT_Passes)
{
	auto test = []() { ASSERT(true); };
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(test_case.Run());
}

TEST_CASE(ASSERT_Fails)
{
	auto test = []() { ASSERT(false); };
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(!test_case.Run());
}

TEST_CASE(ASSERT_TRUE_Passes)
{
	auto test = []() { ASSERT_TRUE(true); };
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(test_case.Run());
}

TEST_CASE(ASSERT_TRUE_Fails)
{
	auto test = []() { ASSERT_TRUE(false); };
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(!test_case.Run());
}

TEST_CASE(ASSERT_FALSE_Passes)
{
	auto test = []() { ASSERT_FALSE(false); };
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(test_case.Run());
}

TEST_CASE(ASSERT_FALSE_Fails)
{
	auto test = []() { ASSERT_FALSE(true); };
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(!test_case.Run());
}

TEST_CASE(ASSERT_THROWS_Passes)
{
	auto test = []() { ASSERT_THROWS(throw std::runtime_error("error")); };
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(test_case.Run());
}

TEST_CASE(ASSERT_THROWS_Fails)
{
	auto test = []() { ASSERT_THROWS((void)0); };
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(!test_case.Run());
}

TEST_CASE(ASSERT_Fails_SecondaryThread)
{
	auto test = []() {
		std::thread secondary_thread([]() { ASSERT(false); });
		secondary_thread.join();
	};
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(!test_case.Run());
}

TEST_CASE(ASSERT_Fails_Destructor)
{
	class DestructorFailure
	{
	public:
		~DestructorFailure() { ASSERT(false); }
	};

	auto test = []() {
		DestructorFailure test;
	};
	TestCaseListItem test_case(test, "TEST_CONSTRUCTOR", "FILE", 1234);

	ASSERT(!test_case.Run());
}

