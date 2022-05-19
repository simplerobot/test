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


TestCaseListItem* TestCaseListItem::g_head = nullptr;
TestCaseListItem** TestCaseListItem::g_tail = &g_head;
static int g_test_depth = 0;
static bool g_test_failure = false;

static thread_local int g_thread_test_depth = 0;


int main(void)
{
	if (!TestCaseListItem::RunAll())
		return 1;
	return 0;
}


class AssertFailedException
{
public:
	AssertFailedException() {}
};


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
	g_test_depth++;
	g_thread_test_depth++;
	g_test_failure = false;
	bool test_passed = false;
	try
	{
		(m_test)();
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

	return test_passed;
}

bool TestCaseListItem::RunAll()
{
	std::ostringstream failed_test_output;

	size_t total_test_count = 0;
	size_t passed_test_count = 0;

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

	std::printf("== TEST SUMMARY ==\n");
	std::printf("%3zd Total Tests\n", total_test_count);
	std::printf("%3zd Tests Passed\n", passed_test_count);
	if (total_test_count == passed_test_count)
		return true;

	std::printf("%3zd Failed Tests\n", total_test_count - passed_test_count);
	std::printf("== Failed Tests ==\n");
	for (char c : failed_test_output.str())
		std::putchar(c);
	std::printf("== End Failed Tests ==\n");
	return false;
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
			::write(m_saved_output, buffer, size);
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

static const char* ShortFileName(const char* filename)
{
	const char* result = filename;
	for (const char* cursor = filename; *cursor != 0; cursor++)
		if (*cursor == '/' || *cursor == '\\')
			result = cursor + 1;
	return result;
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
	va_end(args);
	std::printf("%s '", failure_type);
	std::vprintf(message, args);
	std::printf("' %s %s:%ld\n", function, ShortFileName(file), line);

	if (throw_error)
	{
		throw AssertFailedException();
	}
}

