/*
 * proc_prof.c 由 proc.c 拆分而来, 20230526
 */

#include "proc.h"
#include "stack.h"
#include "arch_amd64.h"
#include "malloc.h"

// caller:
// 	1. runtime·tracebackothers()
// 	2. src/pkg/runtime/panic.c -> runtime·dopanic()
// 	3. src/pkg/runtime/mprof.goc -> Stack()
void runtime·goroutineheader(G *gp)
{
	int8 *status;

	switch(gp->status) {
	case Gidle:
		status = "idle";
		break;
	case Grunnable:
		status = "runnable";
		break;
	case Grunning:
		status = "running";
		break;
	case Gsyscall:
		status = "syscall";
		break;
	case Gwaiting:
		if(gp->waitreason)
			status = gp->waitreason;
		else
			status = "waiting";
		break;
	default:
		status = "???";
		break;
	}
	runtime·printf("goroutine %D [%s]:\n", gp->goid, status);
}

// caller:
// 	1. src/pkg/runtime/panic.c -> runtime·dopanic()
// 	2. src/pkg/runtime/mprof.goc -> Stack()
// 	3. src/pkg/runtime/signal_amd64.c -> runtime·sighandler()
void runtime·tracebackothers(G *me)
{
	G *gp;
	int32 traceback;

	traceback = runtime·gotraceback(nil);
	
	// Show the current goroutine first, if we haven't already.
	if((gp = m->curg) != nil && gp != me) {
		runtime·printf("\n");
		runtime·goroutineheader(gp);
		runtime·traceback(gp->sched.pc, gp->sched.sp, gp->sched.lr, gp);
	}

	for(gp = runtime·allg; gp != nil; gp = gp->alllink) {
		if(gp == me || gp == m->curg || gp->status == Gdead) {
			continue;
		}
		if(gp->issystem && traceback < 2) {
			continue;
		}
		runtime·printf("\n");
		runtime·goroutineheader(gp);
		if(gp->status == Grunning) {
			runtime·printf("\tgoroutine running on other thread; stack unavailable\n");
			runtime·printcreatedby(gp);
		} else {
			runtime·traceback(gp->sched.pc, gp->sched.sp, gp->sched.lr, gp);
		}
	}
}

// Called from syscall package before fork.
void syscall·runtime_BeforeFork(void)
{
	// Fork can hang if preempted with signals frequently enough (see issue 5517).
	// Ensure that we stay on the same M where we disable profiling.
	m->locks++;
	if(m->profilehz != 0) {
		runtime·resetcpuprofiler(0);
	} 
}

// Called from syscall package after fork in parent.
void syscall·runtime_AfterFork(void)
{
	int32 hz;

	hz = runtime·sched.profilehz;
	if(hz != 0) {
		runtime·resetcpuprofiler(hz);
	}
	m->locks--;
}

static struct {
	Lock;
	void (*fn)(uintptr*, int32);
	int32 hz;
	uintptr pcbuf[100];
} prof;

static void System(void)
{
}

// caller:
// 	1. src/pkg/runtime/signal_amd64.c -> runtime·sighandler()
//
// Called if we receive a SIGPROF signal.
void runtime·sigprof(uint8 *pc, uint8 *sp, uint8 *lr, G *gp)
{
	int32 n;
	bool traceback;

	if(prof.fn == nil || prof.hz == 0) {
		return;
	}
	traceback = true;

	// Windows does profiling in a dedicated thread w/o m.
	if(!Windows && (m == nil || m->mcache == nil)) {
		traceback = false;
	}

	// Define that a "user g" is a user-created goroutine, and a "system g"
	// is one that is m->g0 or m->gsignal. We've only made sure that we
	// can unwind user g's, so exclude the system g's.
	//
	// It is not quite as easy as testing gp == m->curg (the current user g)
	// because we might be interrupted for profiling halfway through a
	// goroutine switch. The switch involves updating three (or four) values:
	// g, PC, SP, and (on arm) LR. The PC must be the last to be updated,
	// because once it gets updated the new g is running.
	//
	// When switching from a user g to a system g, LR is not considered live,
	// so the update only affects g, SP, and PC. Since PC must be last, there
	// the possible partial transitions in ordinary execution are (1) g alone is updated,
	// (2) both g and SP are updated, and (3) SP alone is updated.
	// If g is updated, we'll see a system g and not look closer.
	// If SP alone is updated, we can detect the partial transition by checking
	// whether the SP is within g's stack bounds. (We could also require that SP
	// be changed only after g, but the stack bounds check is needed by other
	// cases, so there is no need to impose an additional requirement.)
	//
	// There is one exceptional transition to a system g, not in ordinary execution.
	// When a signal arrives, the operating system starts the signal handler running
	// with an updated PC and SP. The g is updated last, at the beginning of the
	// handler. There are two reasons this is okay. First, until g is updated the
	// g and SP do not match, so the stack bounds check detects the partial transition.
	// Second, signal handlers currently run with signals disabled, so a profiling
	// signal cannot arrive during the handler.
	//
	// When switching from a system g to a user g, there are three possibilities.
	//
	// First, it may be that the g switch has no PC update, because the SP
	// either corresponds to a user g throughout (as in runtime.asmcgocall)
	// or because it has been arranged to look like a user g frame
	// (as in runtime.cgocallback_gofunc). In this case, since the entire
	// transition is a g+SP update, a partial transition updating just one of 
	// those will be detected by the stack bounds check.
	//
	// Second, when returning from a signal handler, the PC and SP updates
	// are performed by the operating system in an atomic update, so the g
	// update must be done before them. The stack bounds check detects
	// the partial transition here, and (again) signal handlers run with signals
	// disabled, so a profiling signal cannot arrive then anyway.
	//
	// Third, the common case: it may be that the switch updates g, SP, and PC
	// separately, as in runtime.gogo.
	//
	// Because runtime.gogo is the only instance, we check whether the PC lies
	// within that function, and if so, not ask for a traceback. This approach
	// requires knowing the size of the runtime.gogo function, which we
	// record in arch_*.h and check in runtime_test.go.
	//
	// There is another apparently viable approach, recorded here in case
	// the "PC within runtime.gogo" check turns out not to be usable.
	// It would be possible to delay the update of either g or SP until immediately
	// before the PC update instruction. Then, because of the stack bounds check,
	// the only problematic interrupt point is just before that PC update instruction,
	// and the sigprof handler can detect that instruction and simulate stepping past
	// it in order to reach a consistent state. On ARM, the update of g must be made
	// in two places (in R10 and also in a TLS slot), so the delayed update would
	// need to be the SP update. The sigprof handler must read the instruction at
	// the current PC and if it was the known instruction (for example, JMP BX or 
	// MOV R2, PC), use that other register in place of the PC value.
	// The biggest drawback to this solution is that it requires that we can tell
	// whether it's safe to read from the memory pointed at by PC.
	// In a correct program, we can test PC == nil and otherwise read,
	// but if a profiling signal happens at the instant that a program executes
	// a bad jump (before the program manages to handle the resulting fault)
	// the profiling handler could fault trying to read nonexistent memory.
	//
	// To recap, there are no constraints on the assembly being used for the
	// transition. We simply require that g and SP match and that the PC is not
	// in runtime.gogo.
	//
	// On Windows, one m is sending reports about all the g's, so gp == m->curg
	// is not a useful comparison. The profilem function in os_windows.c has
	// already checked that gp is a user g.
	if(gp == nil ||
	   (!Windows && gp != m->curg) ||
	   (uintptr)sp < gp->stackguard - StackGuard || gp->stackbase < (uintptr)sp ||
	   ((uint8*)runtime·gogo <= pc && pc < (uint8*)runtime·gogo + RuntimeGogoBytes)) {
		traceback = false;
	}

	// Race detector calls asmcgocall w/o entersyscall/exitsyscall,
	// we can not currently unwind through asmcgocall.
	if(m != nil && m->racecall) {
		traceback = false;
	} 

	runtime·lock(&prof);
	if(prof.fn == nil) {
		runtime·unlock(&prof);
		return;
	}
	n = 0;
	if(traceback) {
		n = runtime·gentraceback(
			(uintptr)pc, (uintptr)sp, (uintptr)lr, gp, 0, 
			prof.pcbuf, nelem(prof.pcbuf), nil, nil, false
		);
	}
	if(!traceback || n <= 0) {
		n = 2;
		prof.pcbuf[0] = (uintptr)pc;
		prof.pcbuf[1] = (uintptr)System + 1;
	}
	prof.fn(prof.pcbuf, n);
	runtime·unlock(&prof);
}

// caller:
// 	1. src/pkg/runtime/cpuprof.c -> runtime·SetCPUProfileRate()
//
// Arrange to call fn with a traceback hz times a second.
void runtime·setcpuprofilerate(void (*fn)(uintptr*, int32), int32 hz)
{
	// Force sane arguments.
	if(hz < 0) hz = 0;
	if(hz == 0) fn = nil;
	if(fn == nil) hz = 0;

	// Disable preemption, otherwise we can be rescheduled to another thread
	// that has profiling enabled.
	m->locks++;

	// Stop profiler on this thread so that it is safe to lock prof.
	// if a profiling signal came in while we had prof locked,
	// it would deadlock.
	runtime·resetcpuprofiler(0);

	runtime·lock(&prof);
	prof.fn = fn;
	prof.hz = hz;
	runtime·unlock(&prof);
	runtime·lock(&runtime·sched);
	runtime·sched.profilehz = hz;
	runtime·unlock(&runtime·sched);

	if(hz != 0) {
		runtime·resetcpuprofiler(hz);
	} 

	m->locks--;
}

extern void runtime·morestack(void);

// f 是否已经是处于栈顶的函数(再没有更上层的主调函数了)
// 比如 runtime·goexit()
//
// caller:
// 	1. src/pkg/runtime/traceback_x86.c -> runtime·gentraceback()
//
// Does f mark the top of a goroutine stack?
bool runtime·topofstack(Func *f)
{
	return f->entry == (uintptr)runtime·goexit ||
		f->entry == (uintptr)runtime·mstart ||
		f->entry == (uintptr)runtime·mcall ||
		f->entry == (uintptr)runtime·morestack ||
		f->entry == (uintptr)runtime·lessstack ||
		f->entry == (uintptr)_rt0_go;
}
