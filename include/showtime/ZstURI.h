#pragma once
#include <memory>
#include <ostream>
#include <showtime/ZstExports.h>

#define MAX_PATH_LEN 255
#define DELIM '/'

namespace showtime {

	struct pstr {
		size_t length;
		char* cstr = NULL;
	};

	ZST_EXPORT pstr create_pstr(const char* p);
	ZST_EXPORT pstr create_pstr(const char* p, size_t l);

	class ZstURI
	{
	public:
		ZST_EXPORT ZstURI();
		ZST_EXPORT ~ZstURI();
		ZST_EXPORT ZstURI(const ZstURI& copy);
		ZST_EXPORT ZstURI& operator=(const ZstURI& other);
		ZST_EXPORT ZstURI& operator=(ZstURI&& source);
		ZST_EXPORT ZstURI(const char* path);
		ZST_EXPORT ZstURI(const char* path, size_t len);

		ZST_EXPORT const char* path() const;
		ZST_EXPORT const char* segment(size_t index) const;
		ZST_EXPORT const size_t size() const;
		ZST_EXPORT const size_t full_size() const;

		ZST_EXPORT ZstURI range(size_t start, size_t end) const;
		ZST_EXPORT ZstURI operator+(const ZstURI& other) const;

		//Returns a URI containing the parent
		ZST_EXPORT ZstURI parent() const;

		//Returns a URI containing the first segment
		ZST_EXPORT ZstURI first() const;

		//Return a URI containing the last segment
		ZST_EXPORT ZstURI last() const;

		ZST_EXPORT bool contains(const ZstURI& compare) const;
		ZST_EXPORT static bool equal(const ZstURI& a, const ZstURI& b);
		ZST_EXPORT bool operator==(const ZstURI& other) const;
		ZST_EXPORT bool operator!=(const ZstURI& other) const;
		ZST_EXPORT bool operator< (const ZstURI& b) const;
		ZST_EXPORT bool is_empty() const;


	private:
	
		void init();
		pstr m_original_path;
		int  m_component_count;
		pstr m_components[MAX_PATH_LEN];
	};

	struct ZstURIHash
	{
		ZST_EXPORT size_t operator()(ZstURI const& k) const;
	};

}

namespace std {
	ZST_EXPORT std::ostream& operator<<(std::ostream& os, const showtime::ZstURI& uri);
};