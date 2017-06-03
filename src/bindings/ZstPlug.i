%module(threads="1") ZstPlug
%{
	#include "ZstExports.h"
	#include "ZstPlug.h"
%}

%include <windows.i>
%include "ZstExports.h"
%include "ZstPlug.h"
