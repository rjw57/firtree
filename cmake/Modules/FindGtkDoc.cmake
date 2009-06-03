# CMake macros to find the GtkDoc documentation system

# Output variables:
#
#   GTKDOC_FOUND            ... set to 1 if GtkDoc was foung
#
# If GTKDOC_FOUND == 1:
#
#   GTKDOC_SCAN_EXE         ... the location of the gtkdoc-scan executable
#   GTKDOC_SCANGOBJ_EXE     ... the location of the gtkdoc-scangobj executable
#   GTKDOC_MKTMPL_EXE       ... the location of the gtkdoc-mktmpl executable
#   GTKDOC_MKDB_EXE         ... the location of the gtkdoc-mkdb executable
#   GTKDOC_MKHTML_EXE       ... the location of the gtkdoc-mkhtml executable
#   GTKDOC_FIXXREF_EXE      ... the location of the gtkdoc-fixxref executable

set(GTKDOC_FOUND 1)

find_program(GTKDOC_SCAN_EXE gtkdoc-scan)
if(NOT GTKDOC_SCAN_EXE)
	message(STATUS "gtkdoc-scan not found")
    set(GTKDOC_FOUND 0)
endif(NOT GTKDOC_SCAN_EXE)

find_program(GTKDOC_SCANGOBJ_EXE gtkdoc-scangobj)
if(NOT GTKDOC_SCANGOBJ_EXE)
	message(STATUS "gtkdoc-scangobj not found")
    set(GTKDOC_FOUND 0)
endif(NOT GTKDOC_SCANGOBJ_EXE)

find_program(GTKDOC_MKTMPL_EXE gtkdoc-mktmpl)
if(NOT GTKDOC_MKTMPL_EXE)
	message(STATUS "gtkdoc-mktmpl not found")
    set(GTKDOC_FOUND 0)
endif(NOT GTKDOC_MKTMPL_EXE)

find_program(GTKDOC_MKDB_EXE gtkdoc-mkdb)
if(NOT GTKDOC_MKDB_EXE)
	message(STATUS "gtkdoc-mkdb not found")
    set(GTKDOC_FOUND 0)
endif(NOT GTKDOC_MKDB_EXE)

find_program(GTKDOC_MKHTML_EXE gtkdoc-mkhtml)
if(NOT GTKDOC_MKHTML_EXE)
	message(STATUS "gtkdoc-mkhtml not found")
    set(GTKDOC_FOUND 0)
endif(NOT GTKDOC_MKHTML_EXE)

find_program(GTKDOC_FIXXREF_EXE gtkdoc-fixxref)
if(NOT GTKDOC_FIXXREF_EXE)
	message(STATUS "gtkdoc-fixxref not found")
    set(GTKDOC_FOUND 0)
endif(NOT GTKDOC_FIXXREF_EXE)

# vim:sw=4:ts=4:et:autoindent

