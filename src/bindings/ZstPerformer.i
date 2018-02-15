%ignore ZstPerformer::read;
%ignore ZstPerformer::write;

%inline %{
	ZstPerformer* cast_to_performer(ZstEntityBase * entity){
		return dynamic_cast<ZstPerformer*>(entity);
	}
%}


%nodefaultctor ZstPerformer;
%include <entities/ZstPerformer.h>
