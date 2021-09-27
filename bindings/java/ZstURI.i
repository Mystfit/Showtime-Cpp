namespace showtime {

    %javamethodmodifiers ZstURI::hashCode() const %{@Override
      public%};

    %javamethodmodifiers ZstURI::toString() const %{@Override
      public%};


    %extend ZstURI
    {
        const int ZstURI::hashCode() const
        {
            return static_cast<int>(ZstURIHash{}(*self));
        }

        const char * ZstURI::toString() const
        {
            return (self->path()) ? self->path() : "";
        }
    };
}
