PIN_HOME ?= ..
include $(PIN_HOME)/makefile.gnu.config
LINKER?=${CXX}
CXXFLAGS ?=  -Wno-unknown-pragmas $(DBG) $(OPT)

CXX=g++
TOOL_ROOTS = all_pred_mt 
TOOLS = $(TOOL_ROOTS:%=$(OBJDIR)%$(PINTOOL_SUFFIX))


all: $(OBJDIR)
	-$(MAKE) make_all

make_all: $(TOOLS)
#all: cond_full_mt cond_pin cond_full_mt_work
#$(APPS): $(OBJDIR)make-directory

$(OBJDIR)make-directory:
	mkdir -p $(OBJDIR)
	touch $(OBJDIR)make-directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)%.o : %.cpp all_pred.h $(OBJDIR)make-directory
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

$(TOOLS): $(PIN_LIBNAMES)

$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

# cond_pin.o: predictor.h cond_pin.cpp
#	$(CXX) -DVERBOSE -g -c $(CXXFLAGS) $(PIN_CXXFLAGS) cond_pin.cpp -o cond_pin.o
#
# cond_pin: predictor.h cond_pin.o 
#	$(CXX) -g $(PIN_LDFLAGS) $(LINK_DEBUG) cond_pin.o -o cond_pin.so $(PIN_LPATHS) $(PIN_LIBS) $(DBG)
#
# cond_full_mt.o: predictor.h cond_full_mt.cpp
#	$(CXX) -DVERBOSE -g -c $(CXXFLAGS) $(PIN_CXXFLAGS) cond_full_mt.cpp -o cond_full_mt.o
#
# cond_full_mt: predictor.h cond_full_mt.o 
#	$(CXX) -g $(PIN_LDFLAGS) $(LINK_DEBUG) cond_full_mt.o -o cond_full_mt.so $(PIN_LPATHS) $(PIN_LIBS) $(DBG)
# 
# cond_full_mt_work.o: predictor.h cond_full_mt_work.cpp
#	$(CXX) -DVERBOSE -g -c $(CXXFLAGS) $(PIN_CXXFLAGS) cond_full_mt_work.cpp -o cond_full_mt_work.o
#
# cond_full_mt: predictor.h cond_full_mt_work.o 
#	$(CXX) -g $(PIN_LDFLAGS) $(LINK_DEBUG) cond_full_mt_work.o -o cond_full_mt_work.so $(PIN_LPATHS) $(PIN_LIBS) $(DBG)
clean:
	-rm -rf $(OBJDIR) *.out *.log *.tested *.failed *.makefile.copy *.out.*.*
