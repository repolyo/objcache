# FIXME
d := tests


# there has to be some way to make this be less work.

#
# rules to build individual test programs go here.
#
# each executable is described by four lines of make code:
#
# PROGRAM := $(d)/program                  # give it a typeable name
# $(PROGRAM)_HELP := Helpful description   # describe to 'make help' user
# $(PROGRAM)_OBJECTS := $(d)/file1.o ...   # list object files
# $(PROGRAM) : $($(PROGRAM)_OBJECTS)       # program is made of object files
# CLEANFILES += outputfile                 # clean up output files, if any


TEST_LLOG := $d/test_llog
TEST_LERROR := $d/test_lerror
TEST_LMEMORY := $d/test_lmemory
TEST_LTHREAD := $d/test_lthread
TEST_LATOMIC := $d/test_latomic
TEST_LSTRFUNCS := $d/test_lstrfuncs
TEST_LSLIST := $d/test_lslist
TEST_LHASH := $d/test_lhash
TEST_LSTR := $d/test_lstr
TEST_LNAME := $d/test_lname
TEST_LDICT := $d/test_ldict
TEST_LVALUE := $d/test_lvalue
TEST_LDEBUG := $d/test_ldebug
TEST_LSTACK := $d/test_lstack
TEST_LCACHE := $d/test_lcache
BENCH_LNAME := $d/bench_lname

#
# TEST_PROGRAMS - programs to be built in the test dir
#    These are programs that will be compiled and linked, but not
#    installed or used outside of the build process.
#    Note that this is very distinct from TESTS; see below.
#
# these are listed longhand to make it easy to comment out single tests
TEST_PROGRAMS := 
TEST_PROGRAMS += $(TEST_LLOG)
TEST_PROGRAMS += $(TEST_LERROR)
TEST_PROGRAMS += $(TEST_LMEMORY)
TEST_PROGRAMS += $(TEST_LTHREAD)
TEST_PROGRAMS += $(TEST_LATOMIC)
TEST_PROGRAMS += $(TEST_LSTRFUNCS)
TEST_PROGRAMS += $(TEST_LSLIST)
TEST_PROGRAMS += $(TEST_LSTACK)
TEST_PROGRAMS += $(TEST_LCACHE)
TEST_PROGRAMS += $(TEST_LHASH)
TEST_PROGRAMS += $(TEST_LSTR)
TEST_PROGRAMS += $(TEST_LNAME)
TEST_PROGRAMS += $(TEST_LDICTOBJ)
TEST_PROGRAMS += $(TEST_LDICT)
TEST_PROGRAMS += $(TEST_LVALUE)
TEST_PROGRAMS += $(TEST_LDEBUG)

TEST_PROGRAMS += $(BENCH_LNAME)


# these runtime path things are stolen from perl's makefiles, so we don't
# have to set LDLIBRARYPATH
OUTPUTDIR := $(CURDIR)
$(TEST_PROGRAMS) : LD_RUN_PATH = $(OUTPUTDIR)
$(TEST_PROGRAMS) : LDFLAGS += -Wl,-rpath,$(LD_RUN_PATH)
$(TEST_PROGRAMS) : LOADLIBES += \
	-L$(OUTPUTDIR) -lllib $(THREAD_LIBS) -lm


# all of the programs need libllib.
$(TEST_PROGRAMS) : libllib$(LIBEXT)


TARGETS += $(TEST_PROGRAMS)
CLEANFILES += $(TEST_PROGRAMS) $(TEST_OBJECTS)


#
# TESTS - list of test programs to run
#    This is a list of all the test programs we will try to run in
#    'make check'.  This may include scripts that run and check the
#    output of other programs built, so it's not necessarily the
#    same as TEST_PROGRAMS.
#
#TESTS = $(TEST_PROGRAMS)
DONT_RUN = $(BENCH_LNAME)
ifneq ($(strip $(CROSS_COMPILE)),)
DONT_RUN += $(TEST_LTHREAD) # qemu doesn't handle threads in process mode
endif
TESTS = $(filter-out $(DONT_RUN), $(TEST_PROGRAMS))


check-force:
	$(MAKE) FORCE_CHECK=yes check;

# clean up the log files created by the test runner
CLEANFILES += $(TESTS:%=%.log)



include $(BUILDUTIL_DIR)/check.mak
