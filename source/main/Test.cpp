#ifdef TEST

#include "Test.hpp"
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <cxxabi.h>
#include <unistd.h>
#include <sstream>
#include <thread>
#include <chrono>
#include "logger.h"


TestCaseListItem* TestCaseListItem::g_head = nullptr;
TestCaseListItem** TestCaseListItem::g_tail = &g_head;

TestHelperListItem* TestHelperListItem::g_head = nullptr;
TestHelperListItem** TestHelperListItem::g_tail = &g_head;


static int g_test_depth = 0;
static bool g_test_failure = false;

static thread_local int g_thread_test_depth = 0;


int main(void)
{
	if (!TestCaseListItem::RunAll())
		return 1;
	return 0;
}


extern void __attribute((weak)) logger_format_message(LoggerLevel level, const char* zone, const char* format, ...)
{
	std::printf("%s %s ", ToString(level), zone);

	va_list args;
	va_start(args, format);
	std::vprintf(format, args);
	va_end(args);

	std::printf("\n");
}


class OutputCapture
{
public:
	OutputCapture();
	~OutputCapture();

	std::string Stop();

private:
	std::thread m_redirect_thread;
	int m_saved_output;
	int m_pipe_source;
	std::ostringstream m_buffer;
};


TestCaseListItem::TestCaseListItem(TestCase test, const char* name, const char* file, long line)
	: m_test(test)
	, m_name(name)
	, m_file(file)
	, m_line(line)
	, m_next(nullptr)
	, m_prev(g_tail)
{
	*g_tail = this;
	g_tail = &this->m_next;
}

TestCaseListItem::~TestCaseListItem()
{
	*m_prev = m_next;
}

bool TestCaseListItem::Run()
{
	TestHelperListItem::Run(TestHelperListItem::SETUP);
	g_test_depth++;
	g_thread_test_depth++;
	g_test_failure = false;
	bool test_passed = false;
	try
	{
		TestHelperListItem::Run(TestHelperListItem::START);
		(m_test)();
		TestHelperListItem::Run(TestHelperListItem::FINISH);
		test_passed = !g_test_failure;
	}
	catch (const AssertFailedException& e)
	{
	}
	catch (const std::exception& e)
	{
		int status = 0;
		const char* name_raw = typeid(e).name();
		char* name_demangled = abi::__cxa_demangle(name_raw, 0, 0, &status);
		const char* name = (name_demangled == nullptr) ? name_raw : name_demangled;
		std::printf("FAILED - Test failed with exception %s.  Error: %s\n", name, e.what());
		std::free(name_demangled);
	}
	catch (...)
	{
		std::printf("FAILED - Test failed with unknown exception.\n");
	}
	g_test_depth--;
	g_thread_test_depth--;
	g_test_failure = false;
	TestHelperListItem::Run(TestHelperListItem::TEARDOWN);

	return test_passed;
}

bool TestCaseListItem::RunAll()
{
	std::printf("== RUNNING TEST CASES ==\n");

	std::ostringstream failed_test_output;

	size_t total_test_count = 0;
	size_t passed_test_count = 0;
	auto start_time = std::chrono::steady_clock::now();

	for (TestCaseListItem* current_test = g_head; current_test != nullptr; current_test = current_test->m_next)
	{
		std::printf("=== TEST: %s ===\n", current_test->m_name);

		OutputCapture capture;

		std::fflush(stdout);
		total_test_count++;

		if (current_test->Run())
		{
			passed_test_count++;
		}
		else
		{
			failed_test_output << "=== Failed Test: " << current_test->m_name << " ===\n" << capture.Stop();
		}
	}

	auto end_time = std::chrono::steady_clock::now();
	double elapsed_time_s = std::chrono::duration<double, std::ratio<1, 1>>(end_time - start_time).count();

	if (total_test_count != passed_test_count)
	{
		std::printf("== Failed Tests ==\n");
		for (char c : failed_test_output.str())
			std::putchar(c);
		std::printf("== End Failed Tests ==\n");
	}

	std::printf("== TEST SUMMARY ==\n");
	std::printf("%3zd Total Tests\n", total_test_count);
	std::printf("%3zd Tests Passed\n", passed_test_count);
	if (total_test_count == passed_test_count)
	{
		std::printf("== TESTS PASSED in %.3fs ==\n", elapsed_time_s);
		return true;
	}
	else
	{
		std::printf("%3zd Failed Tests\n", total_test_count - passed_test_count);
		std::printf("== TESTS FAILED in %.3fs ==\n", elapsed_time_s);
		return false;
	}
}


TestHelperListItem::TestHelperListItem(HelperFn fn, Type type)
	: m_helper_fn(fn)
	, m_type(type)
	, m_next(nullptr)
	, m_prev(g_tail)
{
	*g_tail = this;
	g_tail = &this->m_next;
}

TestHelperListItem::~TestHelperListItem()
{
	*m_prev = m_next;
}

void TestHelperListItem::Run(Type type)
{
	for (TestHelperListItem* cursor = g_head; cursor != nullptr; cursor = cursor->m_next)
		if (cursor->m_type == type)
			(*cursor->m_helper_fn)();
}


OutputCapture::OutputCapture()
	: m_saved_output(::dup(1))
{
	std::fflush(stdout);

	int pipe_fd[2];
	if (::pipe(pipe_fd) != 0)
		throw std::runtime_error("Unable to create pipe to capture test output.");

	int pipe_read = pipe_fd[0];
	int pipe_write = pipe_fd[1];
	m_pipe_source = pipe_read;

	::dup2(pipe_write, 1);
	::close(pipe_write);

	m_redirect_thread = std::thread([&] {
		char buffer[1024];
		ssize_t size = ::read(m_pipe_source, buffer, sizeof(buffer));
		while (size > 0)
		{
			if (::write(m_saved_output, buffer, size) < 0)
				break;
			m_buffer.write(buffer, size);
			size = ::read(m_pipe_source, buffer, sizeof(buffer));
		}
		::close(m_pipe_source);
	});
}

OutputCapture::~OutputCapture()
{
	Stop();
}

std::string OutputCapture::Stop()
{
	if (m_redirect_thread.joinable())
	{
		std::fflush(stdout);
		::dup2(m_saved_output, 1);
		m_redirect_thread.join();
		::close(m_saved_output);
	}
	return m_buffer.str();
}

static bool is_destructor(const char* function_name)
{
	return (std::strstr(function_name, "::~") != nullptr);
}

extern void NotifyAssertFailed(const char* file, long line, const char* function, const char* message, ...)
{
	bool throw_error = true;
	const char* failure_type = "ASSERT FAILED";

	if (g_test_depth == 0)
	{
		failure_type = "ASSERT FAILED OUTSIDE TESTS";
		throw_error = false;
	}

	if (g_test_depth > 1)
	{
		failure_type = "NESTED ASSERT FAILED";
	}

	if (g_thread_test_depth == 0)
	{
		failure_type = "ASSERT FAILED IN SECONDARY THREAD";
		g_test_failure = true;
		throw_error = false;
	}

	if (std::uncaught_exception())
	{
		failure_type = "ASSERT FAILED IN EXCEPTION PROCESSING";
		g_test_failure = true;
		throw_error = false;
	}

	if (is_destructor(function))
	{
		failure_type = "ASSERT FAILED IN DESTRUCTOR";
		g_test_failure = true;
		throw_error = false;
	}

	va_list args;
	va_start(args, message);
	std::printf("%s:%ld:1: assert: ‘", file, line);
	std::vprintf(message, args);
	std::printf("‘ %s ", failure_type);
	if (std::strncmp(function, "TEST_CASE_", 10) == 0)
		std::printf("in test: TEST_CASE(%s)", function + 10);
	else
		std::printf("in function: %s", function);
	std::printf("\n");
	va_end(args);

	if (throw_error)
	{
		throw AssertFailedException();
	}
}

#endif // TEST
