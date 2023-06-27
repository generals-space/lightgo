package runtime

// 	@compatible: 该结构(Frames, Frame)在 v1.7 版本初次添加

// Frames may be used to get function/file/line information for a
// slice of PC values returned by Callers.
type Frames struct {
	callers []uintptr

	// If previous caller in iteration was a panic, then
	// ci.callers[0] is the address of the faulting instruction
	// instead of the return address of the call.
	wasPanic bool

	// Frames to return for subsequent calls to the Next method.
	// Used for non-Go frames.
	frames *[]Frame
}

// Frame is the information returned by Frames for each call frame.
type Frame struct {
	// Program counter for this frame; multiple frames may have
	// the same PC value.
	PC uintptr

	// Func for this frame; may be nil for non-Go code or fully
	// inlined functions.
	Func *Func

	// Function name, file name, and line number for this call frame.
	// May be the empty string or zero if not known.
	// If Func is not nil then Function == Func.Name().
	Function string
	File     string
	Line     int

	// Entry point for the function; may be zero if not known.
	// If Func is not nil then Entry == Func.Entry().
	Entry uintptr
}

// CallersFrames takes a slice of PC values returned by Callers and
// prepares to return function/file/line information.
// Do not change the slice until you are done with the Frames.
func CallersFrames(callers []uintptr) *Frames {
	return &Frames{callers: callers}
}

// Next returns frame information for the next caller.
// If more is false, there are no more callers (the Frame value is valid).
func (ci *Frames) Next() (frame Frame, more bool) {
	if ci.frames != nil {
		// We have saved up frames to return.
		f := (*ci.frames)[0]
		if len(*ci.frames) == 1 {
			ci.frames = nil
		} else {
			*ci.frames = (*ci.frames)[1:]
		}
		return f, ci.frames != nil || len(ci.callers) > 0
	}

	if len(ci.callers) == 0 {
		ci.wasPanic = false
		return Frame{}, false
	}
	pc := ci.callers[0]
	ci.callers = ci.callers[1:]
	more = len(ci.callers) > 0
	f := FuncForPC(pc)
	if f == nil {
		ci.wasPanic = false
		/*
		// 	@todo: cgo
		if cgoSymbolizer != nil {
			return ci.cgoNext(pc, more)
		}
		*/
		return Frame{}, more
	}

	entry := f.Entry()
	xpc := pc
	if xpc > entry && !ci.wasPanic {
		xpc--
	}
	file, line := f.FileLine(xpc)

	function := f.Name()
	ci.wasPanic = entry == PcSigpanic()

	frame = Frame{
		PC:       xpc,
		Func:     f,
		Function: function,
		File:     file,
		Line:     line,
		Entry:    entry,
	}

	return frame, more
}

/*

// cgoNext returns frame information for pc, known to be a non-Go function,
// using the cgoSymbolizer hook.
func (ci *Frames) cgoNext(pc uintptr, more bool) (Frame, bool) {
	arg := cgoSymbolizerArg{pc: pc}
	callCgoSymbolizer(&arg)

	if arg.file == nil && arg.funcName == nil {
		// No useful information from symbolizer.
		return Frame{}, more
	}

	var frames []Frame
	for {
		frames = append(frames, Frame{
			PC:       pc,
			Func:     nil,
			Function: gostring(arg.funcName),
			File:     gostring(arg.file),
			Line:     int(arg.lineno),
			Entry:    arg.entry,
		})
		if arg.more == 0 {
			break
		}
		callCgoSymbolizer(&arg)
	}

	// No more frames for this PC. Tell the symbolizer we are done.
	// We don't try to maintain a single cgoSymbolizerArg for the
	// whole use of Frames, because there would be no good way to tell
	// the symbolizer when we are done.
	arg.pc = 0
	callCgoSymbolizer(&arg)

	if len(frames) == 1 {
		// Return a single frame.
		return frames[0], more
	}

	// Return the first frame we saw and store the rest to be
	// returned by later calls to Next.
	rf := frames[0]
	frames = frames[1:]
	ci.frames = new([]Frame)
	*ci.frames = frames
	return rf, true
}

*/

// 	implementAt: src/pkg/runtime/runtime1.goc -> PcSigpanic()
func PcSigpanic() (ret uintptr)
