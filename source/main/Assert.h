#pragma once


#ifdef TEST
	#define ASSERT(X) ((X) ? (void)0 : NotifyAssertFailed(__FILE__, __LINE__, __PRETTY_FUNCTION__, "ASSERT(%s)", #X))
	#define ASSERT_TRUE(X) ((X) ? (void)0 : NotifyAssertFailed(__FILE__, __LINE__, __PRETTY_FUNCTION__, "ASSERT_TRUE(%s)", #X))
	#define ASSERT_FALSE(X) ((X) ? NotifyAssertFailed(__FILE__, __LINE__, __PRETTY_FUNCTION__, "ASSERT_FALSE(%s)", #X) : (void)0)
	#define ASSERT_THROWS(X) { bool caught_error = false; try { X; } catch (...) { caught_error = true; } if (!caught_error) NotifyAssertFailed(__FILE__, __LINE__, __PRETTY_FUNCTION__, "ASSERT_THROWS(%s)", #X); }

	#ifdef __cplusplus
	extern "C" {
	#endif
	extern void NotifyAssertFailed(const char* file, long line, const char* function, const char* message, ...) __attribute__ ((format (printf, 4, 5)));
	#ifdef __cplusplus
	}
	#endif
#else
	#define ASSERT(X)
	#define ASSERT_TRUE(X)
	#define ASSERT_FALSE(X)
	#define ASSERT_THROWS(X)
#endif
