%module(directors="1") showtime
%{
	#include <Showtime.h>
	#include <ShowtimeServer.h>
	#include <schemas/graph_types_generated.h>
	using namespace showtime;
%}

%feature("director:except") {
    if( $error != NULL ) {
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch( &ptype, &pvalue, &ptraceback );
        PyErr_Restore( ptype, pvalue, ptraceback );
        PyErr_Print();
        //Py_Exit(1);
    }
}

%begin %{
	#ifdef _MSC_VER
	// Force MSVC to not link against the debug python lib since it's not included in binary python installs
	#define SWIG_PYTHON_INTERPRETER_NO_DEBUG
	#endif
%}


%include "../preflight.i"
%include "api_extensions.i"
%include "ZstBundle.i"
%include "ZstSynchronisable.i"
%include "ZstEntityBase.i"
%include "ZstComponent.i"
%include "ZstEntityFactory.i"
%include "ZstPerformer.i"
%include "ShowtimeClient.i"
%include "../showtime.i"
