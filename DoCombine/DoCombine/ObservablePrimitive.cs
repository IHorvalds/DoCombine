using System.ComponentModel;

namespace DoCombine
{
    //! @brief Only used to fire PropertyChanged events
    //!        when the encapsulated Object is assigned to.
    //!        Does not care at all about updates to members
    //!        of the Object.
    //! @note  Assign directly to Object property.
    //! @note  Assumes Type has a default constructor.
    public class ObservablePrimitive<Type> : INotifyPropertyChanged
        where Type : new()
    {
        public event PropertyChangedEventHandler? PropertyChanged;
        private Type _object = new Type();
        public Type Object
        {
            get { return _object; }
            set
            {
                _object = value;
                OnPropertyChanged();
            }
        }

        public ObservablePrimitive()
        {
        }

        public ObservablePrimitive(Type obj)
        {
            _object = obj;
        }

        protected void OnPropertyChanged()
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(null));
        }

        public static implicit operator Type(ObservablePrimitive<Type> observable)
        {
            return observable.Object;
        }
    }
}
