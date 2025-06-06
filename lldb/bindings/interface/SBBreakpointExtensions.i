STRING_EXTENSION_OUTSIDE(SBBreakpoint)

%extend lldb::SBBreakpoint {
#ifdef SWIGPYTHON
    %pythoncode %{

        class locations_access(object):
            '''A helper object that will lazily hand out locations for a breakpoint when supplied an index.'''
            def __init__(self, sbbreakpoint):
                self.sbbreakpoint = sbbreakpoint

            def __len__(self):
                if self.sbbreakpoint:
                    return int(self.sbbreakpoint.GetNumLocations())
                return 0

            def __getitem__(self, key):
                if isinstance(key, int):
                    count = len(self)
                    if -count <= key < count:
                        key %= count
                        return self.sbbreakpoint.GetLocationAtIndex(key)
                return None

        def get_locations_access_object(self):
            '''An accessor function that returns a locations_access() object which allows lazy location access from a lldb.SBBreakpoint object.'''
            return self.locations_access (self)

        def get_breakpoint_location_list(self):
            '''An accessor function that returns a list() that contains all locations in a lldb.SBBreakpoint object.'''
            locations = []
            accessor = self.get_locations_access_object()
            for idx in range(len(accessor)):
                locations.append(accessor[idx])
            return locations

        def __iter__(self):
            '''Iterate over all breakpoint locations in a lldb.SBBreakpoint
            object.'''
            return lldb_iter(self, 'GetNumLocations', 'GetLocationAtIndex')

        def __len__(self):
            '''Return the number of breakpoint locations in a lldb.SBBreakpoint
            object.'''
            return self.GetNumLocations()

        locations = property(get_breakpoint_location_list, None, doc='''A read only property that returns a list() of lldb.SBBreakpointLocation objects for this breakpoint.''')
        location = property(get_locations_access_object, None, doc='''A read only property that returns an object that can access locations by index (not location ID) (location = bkpt.location[12]).''')
        id = property(GetID, None, doc='''A read only property that returns the ID of this breakpoint.''')
        enabled = property(IsEnabled, SetEnabled, doc='''A read/write property that configures whether this breakpoint is enabled or not.''')
        one_shot = property(IsOneShot, SetOneShot, doc='''A read/write property that configures whether this breakpoint is one-shot (deleted when hit) or not.''')
        num_locations = property(GetNumLocations, None, doc='''A read only property that returns the count of locations of this breakpoint.''')
        auto_continue = property(GetAutoContinue, SetAutoContinue, doc='A read/write property that configures the auto-continue property of this breakpoint.')
        condition = property(GetCondition, SetCondition, doc='A read/write property that configures the condition of this breakpoint.')
        hit_count = property(GetHitCount, doc='A read only property that returns the hit count of this breakpoint.')
        ignore_count = property(GetIgnoreCount, SetIgnoreCount, doc='A read/write property that configures the ignore count of this breakpoint.')
        queue_name = property(GetQueueName, SetQueueName, doc='A read/write property that configures the queue name criteria of this breakpoint.')
        target = property(GetTarget, doc='A read only property that returns the target of this breakpoint.')
        thread_id = property(GetThreadID, SetThreadID, doc='A read/write property that configures the thread id criteria of this breakpoint.')
        thread_index = property(GetThreadIndex, SetThreadIndex, doc='A read/write property that configures the thread index criteria of this breakpoint.')
        thread_name = property(GetThreadName, SetThreadName, doc='A read/write property that configures the thread name criteria of this breakpoint.')
    %}
#endif
}
