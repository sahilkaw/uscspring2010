COMPILER = g++
CFLAGS = -c
VPATH = src
INCLUDE = -Iinclude
LIB = -lsocket -lnsl -lresolv

all : Branch Database User

User: User.c
	$(COMPILER) $(INCLUDE) $(LIB) $^ -o $@

Database : Database.c
	$(COMPILER) $(INCLUDE) $(LIB) $^ -o $@

Branch : Branch.c
	$(COMPILER) $(INCLUDE) $(LIB) $^ -o $@

clean: 
	rm -rf *.o *.out *~ src/*~ include/*~
