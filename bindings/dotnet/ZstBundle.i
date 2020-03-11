namespace showtime {
  // MACRO for use within the ZstBundle class body
  %define ZSTBUNDLE_MINIMUM(CSINTERFACE, CONST_REFERENCE, CTYPE...)
  %typemap(csinterfaces) ZstBundle< CTYPE > "global::System.IDisposable, global::System.Collections.IEnumerable\n    , global::System.Collections.Generic.CSINTERFACE<$typemap(cstype, CTYPE)>\n";
  %proxycode %{
    public $typemap(cstype, CTYPE) this[int index]  {
      get {
        return getitem(index);
      }
    }

    public int Count {
      get {
        return (int)size();
      }
    }

    global::System.Collections.Generic.IEnumerator<$typemap(cstype, CTYPE)> global::System.Collections.Generic.IEnumerable<$typemap(cstype, CTYPE)>.GetEnumerator() {
      return new $csclassnameEnumerator(this);
    }

    global::System.Collections.IEnumerator global::System.Collections.IEnumerable.GetEnumerator() {
      return new $csclassnameEnumerator(this);
    }

    public $csclassnameEnumerator GetEnumerator() {
      return new $csclassnameEnumerator(this);
    }

    // Type-safe enumerator
    /// Note that the IEnumerator documentation requires an InvalidOperationException to be thrown
    /// whenever the collection is modified. This has been done for changes in the size of the
    /// collection but not when one of the elements of the collection is modified as it is a bit
    /// tricky to detect unmanaged code that modifies the collection under our feet.
    public sealed class $csclassnameEnumerator : global::System.Collections.IEnumerator
      , global::System.Collections.Generic.IEnumerator<$typemap(cstype, CTYPE)>
    {
      private $csclassname collectionRef;
      private int currentIndex;
      private object currentObject;
      private int currentSize;

      public $csclassnameEnumerator($csclassname collection) {
        collectionRef = collection;
        currentIndex = -1;
        currentObject = null;
        currentSize = collectionRef.Count;
      }

      // Type-safe iterator Current
      public $typemap(cstype, CTYPE) Current {
        get {
          if (currentIndex == -1)
            throw new global::System.InvalidOperationException("Enumeration not started.");
          if (currentIndex > currentSize - 1)
            throw new global::System.InvalidOperationException("Enumeration finished.");
          if (currentObject == null)
            throw new global::System.InvalidOperationException("Collection modified.");
          return ($typemap(cstype, CTYPE))currentObject;
        }
      }

      // Type-unsafe IEnumerator.Current
      object global::System.Collections.IEnumerator.Current {
        get {
          return Current;
        }
      }

      public bool MoveNext() {
        int size = collectionRef.Count;
        bool moveOkay = (currentIndex+1 < size) && (size == currentSize);
        if (moveOkay) {
          currentIndex++;
          currentObject = collectionRef[currentIndex];
        } else {
          currentObject = null;
        }
        return moveOkay;
      }

      public void Reset() {
        currentIndex = -1;
        currentObject = null;
        if (collectionRef.Count != currentSize) {
          throw new global::System.InvalidOperationException("Collection modified.");
        }
      }

      public void Dispose() {
          currentIndex = -1;
          currentObject = null;
      }
    }
  %}

    public:
      ZstBundle();
      typedef CTYPE value_type;
      size_t size() const;
      typedef CONST_REFERENCE const_reference;
      %extend {
        CONST_REFERENCE getitem(int index) throw (std::out_of_range) {
          if (index>=0 && index<(int)$self->size())
            return (*$self)[index];
          else
            throw std::out_of_range("index");
        }
      }
  %enddef
}

namespace showtime {
  template<class T> 
  class ZstBundle {
      ZSTBUNDLE_MINIMUM(IEnumerable, T const&, T)
  };
}
