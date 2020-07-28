#include <cse/cse_utils.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <string.h>

void assert_test_succeeded(int returnCode)
{
    if (returnCode != 0)
    {
        exit(returnCode);
    }
}

int test_without_variables()
{
    char* value = ExpandEnvironmentVariables("C:/my\\test/path");
    if (strcmp(value, "C:/my\\test/path") != 0)
    {
        return 1;
    }
    return 0;
}

int test_only_variable()
{
    char* value = ExpandEnvironmentVariables("${WAYK_CSE_TEST_VAR}");
    if (strcmp(value, "my_value") != 0)
    {
        return 1;
    }
    return 0;
}


int test_single_variable()
{
    char* value = ExpandEnvironmentVariables("C:/${WAYK_CSE_TEST_VAR}/test");
    if (strcmp(value, "C:/my_value/test") != 0)
    {
        return 1;
    }
    return 0;
}

int test_two_variables()
{
    char* value = ExpandEnvironmentVariables("C:/${WAYK_CSE_TEST_VAR}/${WAYK_CSE_TEST_VAR2}");
    if (strcmp(value, "C:/my_value/my_value2") != 0)
    {
        return 1;
    }
    return 0;
}

int test_unclosed_expression()
{
    char* value = ExpandEnvironmentVariables("C:/${qweqweqwe");
    if (value != 0)
    {
        return 1;
    }
    return 0;
}

int test_invalid_expression()
{
    char* value = ExpandEnvironmentVariables("C:/$qwe");
    if (value != 0)
    {
        return 1;
    }
    return 0;
}

int test_escape()
{
    char* value = ExpandEnvironmentVariables("$${WAYK_CSE_TEST_VAR}");
    if (strcmp(value, "${WAYK_CSE_TEST_VAR}") != 0)
    {
        return 1;
    }
    return 0;
}



int main()
{
    SetEnvironmentVariableW(L"WAYK_CSE_TEST_VAR", L"my_value");
    SetEnvironmentVariableW(L"WAYK_CSE_TEST_VAR2", L"my_value2");

    assert_test_succeeded(test_without_variables());
    assert_test_succeeded(test_only_variable());
    assert_test_succeeded(test_single_variable());
    assert_test_succeeded(test_two_variables());
    assert_test_succeeded(test_unclosed_expression());
    assert_test_succeeded(test_invalid_expression());
    assert_test_succeeded(test_escape());
    return 0;
}