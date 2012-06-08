# Makefile fÅr NewFile.dll

SHELL = /bin/sh
srcdir = .

CC           = icc
LIB_DIR      = $d:/emx/lib
LIBS       =	somtk.lib mmpm2.lib
#LDFLAGS =	-Ge- /Gm+ -G4 /ss -Gl /O+ -Oc+ -Ol-  /Q+ /Tn /Ti
#CFLAGS       =   -Ge- /Gm+ -G4 /ss -Gl /O+ -Oc+ -Ol- /Q+ /Tn /Ti /C
LDFLAGS =	-Ge- /Gm+ -G4 /ss -Gl /O+ -Oc+ -Ol-  /Q+ 
CFLAGS       =   -Ge- /Gm+ -G4 /ss -Gl /O+ -Oc+ -Ol- /Q+ /C

DEFS         = image.obj
MOREINC	=	../include
OBJDIR		=	../o
RESDIR		=	../res

SRC =	candyfld.c

OBJECTS  =	$(OBJDIR)/candyfld.obj $(OBJDIR)/image.obj $(OBJDIR)/misc.obj \
		$(OBJDIR)/settingsprocs.obj $(OBJDIR)/mutex.obj $(OBJDIR)/debug.obj \
		$(OBJDIR)/except.obj

all: $(OBJDIR)/candywps.dll $(OBJDIR)/repclass.exe 

#%.obj:	%.c $(MOREINC)\candyinc.h
#	$(CC) -I $(MOREINC) $(CFLAGS)   $@ $<

$(OBJDIR)/candywps.dll:	candyfld.ih $(OBJECTS) candywps.def $(OBJDIR)/candyfolder.res $(OBJDIR)/candyfolder001.res
	$(CC) $(LDFLAGS) /L /Fm$(basename $@)  /Fe$@ $(OBJECTS) $(LIBS) candywps.def
	-cp -f $@ $(OBJDIR)/candywps001.dll
	rc $(OBJDIR)/candyfolder.res $(OBJDIR)/candywps.dll
	rc $(OBJDIR)/candyfolder001.res $(OBJDIR)/candywps001.dll
	cd ../o && MAPSYM $(notdir $(basename $@))

$(OBJDIR)/candyfolder.res:	$(RESDIR)/candyfolder.rc
	rc -i $(MOREINC) -r $(RESDIR)/candyfolder.rc $(OBJDIR)/candyfolder.res

$(OBJDIR)/candyfolder001.res:	$(RESDIR)/candyfolder001.rc
	rc -i $(MOREINC) -r $(RESDIR)/candyfolder001.rc $(OBJDIR)/candyfolder001.res

$(OBJDIR)/candyfld.obj:	candyfld.c		$(MOREINC)\candyinc.h
	$(CC) -I $(MOREINC) $(CFLAGS)  /Fo$@ $<

$(OBJDIR)/image.obj:	image.c
	$(CC) -I $(MOREINC) $(CFLAGS)  /Fo$@ $<

$(OBJDIR)/misc.obj:	misc.c	$(MOREINC)\candyinc.h
	$(CC) -I $(MOREINC) $(CFLAGS)  /Fo$@ $<

$(OBJDIR)/settingsprocs.obj:	settingsprocs.c	$(MOREINC)\candyinc.h
	$(CC) -I $(MOREINC) $(CFLAGS)  /Fo$@ $<

$(OBJDIR)/mutex.obj:	mutex.c
	$(CC) -I $(MOREINC) $(CFLAGS)  /Fo$@ $<

$(OBJDIR)/debug.obj:	debug.c
	$(CC) -I $(MOREINC) $(CFLAGS)  /Fo$@ $<

$(OBJDIR)/except.obj:	except.c
	$(CC) -I $(MOREINC) $(CFLAGS)  /Fo$@ $<

$(OBJDIR)/repclass.exe:	repclass.c
	gcc -o $@ $<

#$(OBJDIR)/$(OBJECTS):	$(SRC)		$(MOREINC)\candyinc.h
#	$(CC) -I $(MOREINC) $(CFLAGS)  /Fo$@ $<

#%.d:	%.c
#    $(SHELL) -ec 'gcc -M  $< \
#                      | sed '\"s/\($*\)\.o[ :]*/\1.o $@ : /g'\" > $@; \
#                      [ -s $@ ] || rm -f $@'
#

candyfld.ih:	candyfld.idl
	sc -p -v -r -maddstar -mnoint -S128000 -C128000  candyfld.idl
	sc -p -v -r -maddstar -mnoint -S128000 -C128000 -sc candyfld.idl



