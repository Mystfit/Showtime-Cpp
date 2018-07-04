#pragma once

class ZstClientModule {
public:
	virtual void init() = 0;
	virtual void destroy() = 0;
};
