CXXFLAGS=-g -I$$HOME/local/include -I../framework -I../echotestlib -DBUILDTEST
CXX=g++
LDFLAGS=-L$$HOME/local/lib

DEST = multicast
SRCS=$(DEST).cpp
OBJS=$(subst .cpp,.o,$(SRCS))
TESTEXEC = $(DEST)test

all: $(TESTEXEC)

$(TESTEXEC): $(OBJS)
	g++ -Wl,-rpath $$HOME/local/lib -L$$HOME/local/lib  -o $(TESTEXEC) $^ $(LDLIBS)

depend: .depend

.depend: $(SRCS) ../framework/framework.h
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS) $(TESTEXEC) .depend

dist-clean: clean
	$(RM) *~ .dependtool

include .depend

run: all
	export DYLD_LIBRARY_PATH=$$HOME/local/lib
	export LD_LIBRARY_PATH=$$HOME/local/lib
	./$(TESTEXEC)


