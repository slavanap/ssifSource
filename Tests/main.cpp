#include "tests.hpp"

#include <iostream>

int main() {
	try {
		CorrelationTests();
	}
	catch (const test_error& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
