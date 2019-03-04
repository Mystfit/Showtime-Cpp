%csmethodmodifiers ZstURI::GetHashCode "public override";
%csmethodmodifiers ZstURI::ToString "public override";

%extend ZstURI
{
    const int ZstURI::GetHashCode() const
    {
        return static_cast<int>(ZstURIHash{}(*self));
    }

    const char * ZstURI::ToString() const
    {
        return (self->path()) ? self->path() : "";
    }
};
