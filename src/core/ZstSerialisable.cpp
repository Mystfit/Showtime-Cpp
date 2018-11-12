#include <ZstSerialisable.h>
#include <nlohmann/json.hpp>

json ZstSerialisable::as_json() const
{
	json obj;
	write_json(obj);
	return obj;
}

std::string ZstSerialisable::as_json_str() const
{
	return as_json().dump();
}

void ZstSerialisable::from_json_str(std::string & str)
{
	read_json(json::parse(str));
}

void ZstSerialisable::from_json_str(const char * str)
{
	read_json(json::parse(str));
}
