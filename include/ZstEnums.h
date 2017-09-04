#pragma once

enum RuntimeLanguage {
	NATIVE_RUNTIME,
	PYTHON_RUNTIME,
	DOTNET_RUNTIME
};

enum ZstValueType {
	ZST_NONE = 0,
	ZST_INT,
	ZST_FLOAT,
	ZST_STRING
};

enum ZstEntityBehaviour {
	FILTER = 0,
	PATCH,
	COMPONENT,
	PERFORMER
};