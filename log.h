#include <stdlib.h>

#define LOGDBG(M, ...) fprintf(stdout, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOGERR(M, ...) fprintf(stdout, "ERROR %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
