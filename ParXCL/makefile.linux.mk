# Makefile for ParX on Linux
# Copyright (c) 1989-2017 M.G.Middelhoek

VERSION = "\"6.5 rev. `date +"%Y-%m-%d"`\""

SYSTEM = LINUX

# targets

PROGRAM = parx

PARXDIR = /usr/local
BDIR = $(PARXDIR)/bin

# Compiler
CC = gcc
CFLAGS = -D$(SYSTEM) -DVERSION=$(VERSION) -Wall -pedantic -O3

# Linker
LINKER = $(CC)
LDFLAGS = -static -z muldefs

# C Code Generators
TM = /usr/local/bin/tm
TMFLAGS =

LEX = flex
LFLAGS = -I

YACC = bison
YFLAGS = -d -v

# Libraries

LIBS = -L/usr/local/lib -ltmc -llapack -lcblas -lblas -lgfortran -lm
TOOLLIBS = -L/usr/local/lib -ltmc -lm

.SUFFIXES: .y .l .t .ds .ht .ct .h .c .o

OBJS= parx.o banner.o \
	actions.o datastruct.o datatpl.o dbase.o dbio.o distance.o \
	error.o extract.o golden.o minbrent.o \
	modes.o modify.o modlib.o newton.o numdat.o \
	objectiv.o parser.o parxlex.o parxyacc.o pprint.o \
	primtype.o prob.o residual.o simulate.o stim2dat.o \
	subset.o vecmat.o readcsv.o cJSON.o jsonio.o \
	mem_func.o bt_func.o prx_func.o prx.o prxinter.o prxcompile.o

MODOBJS= parxmods.o

# .h files generated from tm modules
TMHDRS=	primtype.h datastruct.h

# .c files generated from tm modules
TMSRCS=	primtype.c datastruct.c

JUNK = lex.yy.c y.tab.h y.output y.tab.c \
	parxlex.c parxyacc.c parxyacc.h parxyacc.out tm.sts parx.st \
	exin.nb exout.nb simin.nb simout.nb

help :
	@echo " Possible make targets:"
	@echo "all          Create local running programs."
	@echo "parx         Create ParX program."
	@echo "clean        Free disk space."
	@echo "install      Install relevant files."

all	: $(PROGRAM)

$(PROGRAM): $(OBJS) $(MODOBJS)
	$(LINKER) $(LDFLAGS) $(OBJS) $(MODOBJS) $(LIBS) -o $(PROGRAM)

install	: all
	cp $(PROGRAM) $(BDIR)

clean:
	rm -f $(OBJS) $(MODOBJS)
	rm -f $(TMSRCS) $(TMHDRS)
	rm -f $(PROGRAM)
	rm -f $(JUNK)

# make rules

%.o : %.c
	$(CC) $(CFLAGS) -c $<

%.c : %.y
	$(YACC) $(YFLAGS) $<
	mv $*.tab.c $*.c
	mv $*.tab.h $*.h

%.h : %.y
	$(YACC) $(YFLAGS) $<
	mv $*.tab.h $*.h

%.c : %.l
	$(LEX) $(LFLAGS) $<
	mv lex.yy.c $*.c

%.h : %.ht
	$(TM) $(TMFLAGS) $*.ds $< > $*.h

%.c : %.ct
	$(TM) $(TMFLAGS) $*.ds $< > $*.c

# Dependencies

# Parser
parxyacc.c: parxyacc.y
parxyacc.h: parxyacc.y
parxlex.c: parxlex.l

parxlex.o: parx.h error.h parser.h parxyacc.h $(TMHDRS)
parxyacc.o: parx.h error.h actions.h pprint.h parser.h $(TMHDRS)

# Tm sources
primtype.c: primtype.ct primtype.ds primtype.t
primtype.h: primtype.ht primtype.ds primtype.t
datastruct.c: datastruct.ct datastruct.ds datastruct.t
datastruct.h: datastruct.ht datastruct.ds datastruct.t

primtype.o: parx.h error.h primtype.h
datastruct.o: parx.h error.h primtype.h datastruct.h

# ParX
parx.o: parx.h error.h $(TMHDRS)
banner.o: parx.h
actions.o: parx.h error.h parser.h subset.h simulate.h \
	stim2dat.h extract.h actions.h $(TMHDRS)
datatpl.o: parx.h error.h $(TMHDRS)
dbase.o: parx.h error.h dbio.h $(TMHDRS)
dbio.o: parx.h error.h dbio.h $(TMHDRS)
distance.o: parx.h error.h primtype.h vecmat.h \
	golden.h residual.h distance.h
error.o: parx.h error.h parser.h primtype.h
extract.o: parx.h error.h modes.h objectiv.h residual.h extract.h $(TMHDRS)
golden.o: parx.h error.h primtype.h golden.h
parser.o: parx.h error.h parser.h $(TMHDRS)
minbrent.o: parx.h error.h primtype.h minbrent.h
modes.o: parx.h error.h primtype.h vecmat.h residual.h minbrent.h \
	modify.h objectiv.h modes.h
modify.o: parx.h error.h primtype.h vecmat.h prob.h objectiv.h modify.h
modlib.o: parx.h error.h primtype.h modlib.h
newton.o: parx.h error.h primtype.h minbrent.h vecmat.h simulate.h newton.h
numdat.o: parx.h error.h prxinter.h $(TMHDRS)
objectiv.o: parx.h error.h vecmat.h residual.h objectiv.h $(TMHDRS)
pprint.o: parx.h error.h pprint.h $(TMHDRS)
prob.o: parx.h primtype.h prob.h
residual.o: parx.h error.h vecmat.h distance.h residual.h $(TMHDRS)
simulate.o: parx.h error.h vecmat.h newton.h simulate.h $(TMHDRS)
stim2dat.o: parx.h error.h stim2dat.h $(TMHDRS)
subset.o: parx.h error.h subset.h $(TMHDRS)
vecmat.o: parx.h error.h primtype.h vecmat.h
readcsv.o: parx.h error.h $(TMHDRS)
cJSON.o: cJSON.h
jsonio.o: parx.h error.h cJSON.h $(TMHDRS)

# Model Compiler

prx.o: prx_def.h mem_def.h bt_def.h parx.h
mem_func.o: mem_def.h
bt_func.o: bt_def.h mem_def.h
prxinter.o: prx_def.h mem_def.h error.h prxinter.h $(TMHDRS)
prxcompile.o: prx_def.h mem_def.h error.h parx.h primtype.h

# Build-in Models

parxmods.o: parx.h primtype.h modlib.h $(TMHDRS)

# END OF MAKEFILE #
