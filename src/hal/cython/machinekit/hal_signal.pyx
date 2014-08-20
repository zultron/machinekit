from .hal_priv cimport hal_data_u
from .hal_util cimport hal2py, py2hal, shmptr, valid_dir, valid_type

cdef class Signal:
    cdef hal_sig_t *_sig
    cdef int _handle
    cdef hal_data_u *_storage

    def _alive_check(self):
        if self._handle != self._sig.handle:
            # the underlying HAL signal was deleted.
            raise RuntimeError("link: underlying HAL signal already deleted")

    def __init__(self, char *name, type=HAL_TYPE_UNSPECIFIED):
        hal_required()
        # if no type given, wrap existing signal
        if type == HAL_TYPE_UNSPECIFIED:

            with HALMutex():
                self._sig = halpr_find_sig_by_name(name)
                if self._sig == NULL:
                    raise RuntimeError("signal %s does not exist" % name)

        else:
            r = hal_signal_new(name, type)
            if r:
                raise RuntimeError("Failed to create signal %s: %s" % (name, hal_lasterror()))

            with HALMutex():
                self._sig = halpr_find_sig_by_name(name)
                if self._sig == NULL:
                    raise RuntimeError("BUG: couldnt lookup signal %s" % name)

        self._storage = <hal_data_u *>shmptr(self._sig.data_ptr)
        self._handle = self._sig.handle  # memoize for liveness check

    def link(self, *pins):
        self._alive_check()
        for p in pins:
            if isinstance(p, Pin):
                p.link(self._sig.name)
            else:
                r = hal_link(self._p, self._sig.name)
                if r:
                    raise RuntimeError("Failed to link pin %s to %s: %s" % (p, self._sig.name, hal_lasterror()))

    def delete(self):
        # this will cause a handle mismatch if later operating on a deleted signal wrapper
        r = hal_signal_delete(self._sig.name)
        if (r < 0):
            raise RuntimeError("Fail to delete signal %s: %s" % (self._name, hal_lasterror()))

    def set(self, v):
        self._alive_check()
        if self._sig.writers > 0:
            raise RuntimeError("Signal %s already as %d writer(s)" %
                                      (self._sig.name, self._sig.writers))
        return py2hal(self._sig.type, self._storage, v)

    def get(self):
        self._alive_check()
        return hal2py(self._sig.type, self._storage)


    def pins(self):
        ''' return a list of Pin objects linked to this signal '''
        cdef hal_pin_t *p = NULL

        self._alive_check()
        # need to do this two-step due to HALmutex
        # pins.__get_item__ will aquire the lock, and if we do so again
        # by calling  pins.__get_item_ we'll produce a deadlock
        # solution: collect the names under mutex, then collect wrappers from names
        pinnames = []
        with HALMutex():
            p = halpr_find_pin_by_sig(self._sig,p)
            while p != NULL:
                pinnames.append(p.name)
                p = halpr_find_pin_by_sig(self._sig, p)

        pinlist = []
        for n in pinnames:
            pinlist.append(pins[n])
        return pinlist


    property name:
        def __get__(self):
            self._alive_check()
            return self._sig.name

    property writername:
        def __get__(self):
            cdef char *name
            self._alive_check()
            return modifier_name(self._sig, HAL_OUT)

    property bidirname:
        def __get__(self):
            cdef char *name
            self._alive_check()
            return modifier_name(self._sig, HAL_IO)

    property type:
        def __get__(self):
             self._alive_check()
             return self._sig.type

    property readers:
        def __get__(self):
             self._alive_check()
             return self._sig.readers

    property writers:
        def __get__(self):
             self._alive_check()
             return self._sig.writers

    property bidirs:
        def __get__(self):
            self._alive_check()
            return self._sig.bidirs

    property handle:
        def __get__(self):
            self._alive_check()
            return self._sig.handle


cdef modifier_name(hal_sig_t *sig, int dir):
     cdef hal_pin_t *pin
     cdef int next

     with HALMutex():
         next = hal_data.pin_list_ptr
         while next != 0:
             pin = <hal_pin_t *>shmptr(next)
             if <hal_sig_t *>shmptr(pin.signal) == sig and pin.dir == dir:
                 return pin.name
             next = pin.next_ptr
     return None


cdef _new_sig(char *name, int type):
    if not valid_type(type):
        raise TypeError("new_sig: %s - invalid type %d " % (name, type))
    return Signal(name, type)

def new_sig(name,type):
    _new_sig(name,type)
    return signals[name] # add to sigdict
