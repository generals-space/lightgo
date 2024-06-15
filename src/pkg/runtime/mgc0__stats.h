enum {
	GC_DEFAULT_PTR = GC_NUM_INSTR,
	GC_CHAN,

	GC_NUM_INSTR2
};

struct GCSTATS {
	struct {
		uint64 sum;
		uint64 cnt;
	} ptr;
	uint64 nbytes;
	struct {
		uint64 sum;
		uint64 cnt;
		uint64 notype;
		uint64 typelookup;
	} obj;
	uint64 rescan;
	uint64 rescanbytes;
	uint64 instr[GC_NUM_INSTR2];
	uint64 putempty;
	uint64 getfull;
	struct {
		uint64 foundbit;
		uint64 foundword;
		uint64 foundspan;
	} flushptrbuf;
	struct {
		uint64 foundbit;
		uint64 foundword;
		uint64 foundspan;
	} markonly;
};

void cachestats(void);

#define GcpercentUnknown (-2)
