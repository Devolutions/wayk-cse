#ifndef WAYKCSE_CSE_UTILS_H
#define WAYKCSE_CSE_UTILS_H

char* ExpandEnvironmentVariables(const char* input);

char* GetWaykCseOption(int key);
char* GetProductName();

int GetPowerShellPath(char* pathBuffer, int pathBufferPath);
int GetCmdPath(char* pathBuffer, int pathBufferSize);

int RunPowerShellCommand(const char* command);
int RunCmdCommand(const char* command);

int RunWaykNowInitScript(const char* waykModulePath, const char* initScriptPath);

int RmDirRecursively(const char* path);

#endif //WAYKCSE_CSE_UTILS_H
