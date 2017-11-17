/*
 * 2017-11-10
 * api hook header
 */

#include "cpu.h"


/*
 * monitor api:
 * @env: the cpu state
 * @opaque: additional arg pass to the function
 */
void monitor_api(CPUState *env, void *opaque);

/* check current process with target process */
bool is_target_process(CPUState *env);

/*
 * adjust current rdtsc return value with last rdtsc value.
 * @cur_val: current return value of rdtsc
 * @last_val: last return value of rdtsc, the adjusted result
 * will be store in it.
 */
void adjust_rdtsc_val(uint64_t cur_val, uint64_t *last_val);
