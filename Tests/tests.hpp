#include <stdexcept>
#include <sstream>

class test_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

void CorrelationTests();

#define test_assert(actual, expected) do { \
	if (!((actual) == (expected))) { \
		std::stringstream ss; \
		ss << "Test assertion at " << __LINE__ << " in " << __FUNCTION__ << " in " << __FILE__ << "\n" \
			"Actual value: " << actual << "\n" \
			"Expected value: " << expected; \
		throw test_error(ss.str()); \
	} \
} while(0)
