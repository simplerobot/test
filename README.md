# test
Simple C++ Test Harness for Linux

# Including library
```
#include "Test.hpp"
```

# Test Cases
Test Cases are functions defined with the TEST_CASE macro:
```
TEST_CASE(PACKAGE_CASE)
{
}
```

# Assert
Test cases assert conditions with the ASSERT family of macros:
```
ASSERT(true);
ASSERT_TRUE(true);
ASSERT_FALSE(false);
ASSERT_THROWS(throw std::runtime_error("error"));
```

# Special Cases
The test harness does its best to allow:
* ASSERTS in other threads
* ASSERTS in exception processing
* ASSERTS in destructors