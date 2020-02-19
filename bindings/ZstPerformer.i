namespace showtime {
	%ignore ZstPerformer::read;
	%ignore ZstPerformer::write;
	%nodefaultctor ZstPerformer;

	%template(ZstSerialisablePerformer) ZstSerialisable<Performer, PerformerData>;
}

%inline %{
	showtime::ZstPerformer* cast_to_performer(showtime::ZstSynchronisable * synchronisable){
		return dynamic_cast<showtime::ZstPerformer*>(synchronisable);
	}
%}

%include <entities/ZstPerformer.h>
