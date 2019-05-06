%ignore ZstPerformer::read;
%ignore ZstPerformer::write;

%inline %{
	ZstPerformer* cast_to_performer(ZstSynchronisable * synchronisable){
		return dynamic_cast<ZstPerformer*>(synchronisable);
	}
%}


%nodefaultctor ZstPerformer;
%include <entities/ZstPerformer.h>
