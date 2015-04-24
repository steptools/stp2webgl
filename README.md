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



## Building

This package contains a Visual Studio 2013 project file for building.
When using with ST-Developer Personal Edition, set the platform to
either "Release DLL" or "Debug DLL".  There is also an NMAKE makefile
called win32.mak which can be used for command line builds.

The package uses the ST-Developer libraries to read/write STEP files
and mesh the CAD geometry.  A downloadable version that is free for
personal use can be found at:

ST-Developer Personal Edition 
- Download - http://www.steptools.com/products/stdev/personal.html
- API Docs - http://www.steptools.com/support/stdev_docs/


## Extending

Is there a different back end that you would need?  Create your own
`write_foo.cxx` output driver by using one of the existing ones as a
template.  See [CONTRIBUTING.md](CONTRIBUTING.md) if you would like to
send a pull request with your changes.
