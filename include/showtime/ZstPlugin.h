#pragma once

#include <memory>
#include <vector>
#include <showtime/ZstExports.h>
//#include <uthash.h>
#include <showtime/entities/ZstEntityFactory.h>

// A plugin is a collection of factories and entities that are loaded at runtime

namespace showtime 
{
	//struct ZstPluginSetup {
	//	char config_key[32];
	//	char config_val[32];
	//	UT_hash_handle hh;
	//};

	class ZST_CLASS_EXPORTED ZstPlugin {
	public:
		virtual void init(const char* plugin_data_path = "") = 0;
		virtual const char* name() = 0;
		virtual int version_major() = 0;
		virtual int version_minor() = 0;
		virtual int version_patch() = 0;

		void get_factories(showtime::ZstEntityFactoryBundle& bundle) {
			for (auto& f : m_factories) {
				bundle.add(f.get());
			}
		};

	protected:
		void add_factory(std::unique_ptr<ZstEntityFactory>&& factory) {
			m_factories.push_back(std::move(factory));
		}

		const std::vector< std::unique_ptr<showtime::ZstEntityFactory> >& plugin_factories() const {
			return m_factories;
		}

	private:
		std::vector< std::unique_ptr<showtime::ZstEntityFactory> > m_factories;
	};

	typedef std::shared_ptr<showtime::ZstPlugin>(ZstPlugin_create_t)();
}

