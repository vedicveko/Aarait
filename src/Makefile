# Generated automatically from Makefile.in by configure.
# CircleMUD 3.0 Makefile.in - Makefile template used by 'configure'
#

# C compiler to use
CC = gcc

# Path to cxref utility
CXREF = cxref

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = /home/aarait/aarait/bin

CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE)

LIBS =  -lcrypt 

OBJFILES = act.comm.o act.informative.o act.item.o act.movement.o \
	act.other.o act.social.o act.wizard.o alias.o ban.o boards.o \
	class.o comm.o config.o constants.o db.o fight.o genmob.o genobj.o \
	genolc.o genshp.o genwld.o genzon.o graph.o handler.o improved-edit.o \
	interpreter.o limits.o mail.o medit.o mobact.o modify.o oasis.o \
	objsave.o oedit.o olc.o random.o redit.o sedit.o shop.o spec_assign.o \
	spec_procs.o tedit.o utils.o weather.o zedit.o coins.o records.o \
        color.o pigf_procs.o assemblies.o assedit.o asciimap.o \
        ferry.o quest.o

CXREF_FILES = act.comm.c act.informative.c act.item.c act.movement.c \
	act.other.c act.social.c act.wizard.c alias.c ban.c boards.c \
	class.c comm.c config.c constants.c db.c fight.c genmob.c genobj.c \
	genolc.c genshp.c genwld.c genzon.c graph.c handler.c improved-edit.c \
	interpreter.c limits.c mail.c medit.c mobact.c modify.c oasis.c \
	objsave.c oedit.c olc.c random.c redit.c sedit.c shop.c spec_assign.c \
	spec_procs.c tedit.c utils.c weather.c zedit.c coins.c records.c \
        color.c pigf_procs.c assemblies.c assedit.c asciimap.c \
        ferry.c quest.c

default: all

all: 
	$(MAKE) $(BINDIR)/test

utils: 
	(cd util; $(MAKE) all)
circle:
	$(MAKE) $(BINDIR)/test

$(BINDIR)/test : $(OBJFILES)
	$(CC) -o $(BINDIR)/test $(PROFILE) $(OBJFILES) $(LIBS)

clean:
	rm -f *.o
ref:
#
# Create the cross reference files
# Note, this is not meant to be used unless you've installed cxref...
#
	@for file in $(CXREF_FILES) ; do \
	  echo Cross referencing $$file ; \
	  $(CXREF) -D__CXREF__ -xref -Odoc -Ncircle $$file ; \
	done
#
# Create the source files using cxref
#
	@for file in $(CXREF_FILES) ; do \
	   echo Documenting $$file ; \
	   ( cd . ; $(CXREF) -D__CXREF__ -warn-xref -xref -Odoc -Ncircle -html $$file ) ; \
	   rm -f $(DOCS) ; \
	done
#
# Create the index using cxref
#
	@echo Indexing
	@( cd . ; $(CXREF) -D__CXREF__ -index-all -Odoc -Ncircle -html )
	@rm -f $(DOCS)
#
# Make html files for the .h files
#
	@echo Creating .h.html files...
	@for file in *.h ; do \
	  echo $$file ; \
	  cat /home/jelson/mud/htmlh-head $$file /home/jelson/mud/htmlh-tail > doc/$$file.html ; \
	done
# Copy over to the html directory
	cp doc/*.html /home/jelson/public_html/circle/cdp/cxref
	chmod 644 /home/jelson/public_html/circle/cdp/cxref/*.html

# Dependencies for the object files (automagically generated with
# gcc -MM)

depend:
	$(CC) -MM *.c > depend

-include depend
