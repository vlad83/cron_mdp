include Makefile.defs

CFLAGS += $(DEFS)
CXXFLAGS += $(DEFS) 

TARGET = cron_mdp

CXXSRCS = \
	../mdp/Client.cpp \
	../mdp/MutualHeartbeatMonitor.cpp \
	../mdp/ZMQClientContext.cpp \
	../mdp/ZMQIdentity.cpp \
	AtValue.cpp \
	Cron.cpp \
	Monitor.cpp \
	cron.cpp \
	fs.cpp

include Makefile.rules

clean:
	cd ../mdp && make clean
	rm *.o *.elf -f
