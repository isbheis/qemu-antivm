/*
 * 2017-10-30
 * api hook header
 */

#include "cpu.h"


/*
 * monitor memory api:
 * @env: the cpu state
 * @opaque: additional arg pass to the function
 */
void monitor_memory_api(CPUState *env, void *opaque);
