
### substring

Returns a substring of the provided string.

    substring(str, pos [, len])

A substring of `str` starting at position `pos` until the end of the string is returned.

If `pos` is negative, the returned string will start `pos` characters from the end of `str`.

If `len` is provided, a substring of `str` `len` characters long starting at position `pos`
is returned. If `len` is smaller than 0, an empty string is returned.

Examples:

    evql> SELECT substring("foobar", 2);
          -> "oobar"

    evql> SELECT substring("foobar", -3, 2)
          -> "ba"
