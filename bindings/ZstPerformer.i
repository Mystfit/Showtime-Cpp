namespace showtime {
	%nodefaultctor ZstPerformer;

	%template(ZstSerialisablePerformer) ZstSerialisable<Performer, PerformerData>;

	%ignore ZstPerformer::ZstPerformer(Performer);
	%ignore ZstPerformer::serialize;
	%ignore ZstPerformer::deserialize;
	%ignore ZstPerformer::serialize_partial;
	%ignore ZstPerformer::deserialize_partial;
}

%inline %{
	showtime::ZstPerformer* cast_to_performer(showtime::ZstSynchronisable * synchronisable){
		return dynamic_cast<showtime::ZstPerformer*>(synchronisable);
	}
%}

%include <entities/ZstPerformer.h>
