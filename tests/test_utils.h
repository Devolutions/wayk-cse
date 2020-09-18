#ifndef WAYKCSE_TEST_UTILS_H
#define WAYKCSE_TEST_UTILS_H

#include <stdlib.h>

inline void assert_test_succeeded(int returnCode)
{
	if (returnCode != 0)
	{
		exit(returnCode);
	}
}


#endif //WAYKCSE_TEST_UTILS_H
