#pragma once

#include <boost/config.hpp>
#include <memory>
#include <vector>
#include <entities/ZstEntityFactory.h>

// A plugin is a collection of factories and entities that are loaded at runtime

class ZstPlugin {
public:
	virtual void init(const char * root_name) = 0;
	virtual const char * name() = 0;
	virtual int version_major() = 0;
	virtual int version_minor() = 0;
	virtual int version_patch() = 0;

	virtual void get_factories(showtime::ZstEntityBundle & bundle) = 0;

private:
	std::vector< std::unique_ptr<showtime::ZstEntityFactory> >
};
