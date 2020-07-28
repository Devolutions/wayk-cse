#ifndef WAYKCSE_CSE_UTILS_H
#define WAYKCSE_CSE_UTILS_H

char* ExpandEnvironmentVariables(const char* input);
char* GetWaykCseOption(int key);
char* GetWaykCsePathOption(int key);
int GetPowerShellPath(char* pathBuffer, int pathBufferPath);
int RunPowerShellCommand(const char* command);
int RunWaykNowInitScript(const char* waykModulePath, const char* initScriptPath);

#endif //WAYKCSE_CSE_UTILS_H
