stp2webgl
=======

Facet STEP for Lightweight Viewing
--

This command line tool converts STEP CAD data into triangular meshes,
and then writes the result into lightweight files for use in WebGL and
other visualization applications.


The STEP reading and faceting is handled by ST-Developer libraries.
This code traverses the resulting mesh data and writes STL or a simple
webxml format.  If you have a mesh format that you prefer, consider
adding a writer for it and including it as a command line option.


The webxml format description and a javascript client for displaying
it as WebGL can be found at:

 - http://www.steptools.com/support/stdev_docs/stixmesh/XMLFormat.html
 - http://www.steptools.com/demos/


This package has a Visual Studio 2013 project file for building.  When
using with ST-Developer Personal Edition, set the platform to either
"Release DLL" or "Debug DLL".  There is also an NMAKE makefile called
win32.mak which can be used for command line builds.
