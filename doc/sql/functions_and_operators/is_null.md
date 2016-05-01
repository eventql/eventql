### isnull

Returns true iff the value `a` is NULL and false otherwise. I.e. this method will return true only in one single case and that is  when the provided value has the valuetype NULL. This method will return false for other null-like values that are not the exact NULL valuetype, like the numeric zero or the empty string.

    isnull(value)
