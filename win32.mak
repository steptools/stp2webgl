#
# Makefile for Windows 
#
# Call as "NMAKE /f win32.mak"
# 
# Most defaults are brought in by the rose_config file.  This file
# links against the ST-Developer DLLs so it will work with Personal
# Edition.
# 
!include $(ROSE_CONFIG)

EXEC	= stp2webgl.exe

CXX_CFLAGS 	= \
	/DROSE_DLL \
	/I"$(ROSE_INCLUDE)" \
	/I"$(ROSE_INCLUDE)\stpcad" \
	/I"$(ROSE_INCLUDE)\stixmesh" \
	/I"$(ROSE_INCLUDE)\stix"

CXX_LDFLAGS 	= /LIBPATH:"$(ROSE_LIB)"

LIBRARIES 	= \
	stpcad_stixmeshdll.lib stpcad_stixdll.lib stpcaddll.lib \
	p28e2dll.lib rosedll.lib $(CXX_SYSLIBS)

OBJECTS = \
	stp2webgl$o \
	facet_product$o \
	write_stl$o \
	write_webxml$o


#========================================
# Standard Symbolic Targets
#
default: $(EXEC)
install: $(EXEC)

clean: 
	- $(RM) *.obj
	- $(RM) *.exe
	- $(RM) *.exe.manifest

very-clean: 	clean
spotless: 	very-clean

#========================================
# Executables and other targets
#
$(EXEC): $(OBJECTS)
	$(CXX_LINK) /out:$@ $(OBJECTS) $(LIBRARIES)
	$(CXX_EMBED_EXE_MANIFEST)


