// Dummy json interface
namespace nlohmann::json { 
} 

%ignore nlohmann::json;
%ignore write_json;
%ignore read_json;

%include <ZstSerialisable.h>
