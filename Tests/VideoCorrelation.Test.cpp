#include "tests.hpp"

#include <Filter.VideoCorrelation.hpp>

#include <vector>

static std::vector<int>
	values = { 1, 42, 35, 26, 18, 100, 405, -10 },
	pattern = { 405, 18, 100, 26 };

void CorrelationTests() {
	Tools::Correlation<int> c(pattern, 1000, 10);
	size_t position;
	for (position = 0; position < values.size(); ++position) {
		if (c.Next(values[position]))
			break;
	}
	test_assert(position, 3);
}
