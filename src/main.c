#include "engine/core/logger.h"
#include "engine/core/assertion.h"

int main(void) {

	ar_FATAL("Test Message: %f", 3.14f);
	ar_ERROR("Test Message: %f", 3.14f);
	ar_WARNING("Test Message: %f", 3.14f);
	ar_INFO("Test Message: %f", 3.14f);
	ar_DEBUG("Test Message: %f", 3.14f);

	ar_assert(false);

	return 0;
}

