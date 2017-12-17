#include <config.h>
#include <types.h>
#include <asm.h>
#include <test.h>
#include <vregs.h>
#include <gdbstub.h>

void _start(void);
extern uval64 entry_point[];

struct gdb_functions orig_functions;

/* Use alternate versions of function that stub uses to read and write memory.
 * These functions are wrappers, which set the debug flags and then check it.
 * This will tell us if the memory references succeeded or not.  HV will
 * set a bit in the vregs debug field if we try to touch memory we are not
 * allowed to touch.
 */

static uval __gdbstub
write_mem(struct cpu_state *state, char *mem, uval len, const char *buf)
{
	uval ret = 0;

	hype_vregs->debug |= V_DEBUG_FLAG;
	ret = orig_functions.write_to_mem(state, mem, len, buf);

	if (hype_vregs->debug & V_DEBUG_MEM_FAULT) {
		ret = 0;
	}

	hype_vregs->debug = 0;
	return ret;
}

static uval __gdbstub
write_to_packet_mem(struct cpu_state *state, const char *mem, uval len)
{
	uval ret = 0;

	hype_vregs->debug |= V_DEBUG_FLAG;
	ret = orig_functions.write_to_packet(state, mem, len);

	if (hype_vregs->debug & V_DEBUG_MEM_FAULT) {
		ret = 0;
	}

	hype_vregs->debug = 0;
	return ret;
}

static uval __gdbstub
ext_command(const char* cmd)
{
	hprintf("Executing command: %s\n", cmd);
	if (memcmp(cmd, "hype_bkpt", 9) == 0) {
		hcall_debug(NULL, H_BREAKPOINT,0,0,0,0);
	}
	return 0;
}


void _start(void)
{
	hcall_cons_init(0);

	hype_vregs->v_ex_msr = mfmsr();
	hype_vregs->exception_vectors[EXC_V_DEBUG] = (uval64)entry_point;

	orig_functions = gdb_functions;

	gdb_functions.write_to_mem = write_mem;
	gdb_functions.write_to_packet = write_to_packet_mem;
	gdb_functions.ext_command = ext_command;
}
