
### substring

Returns a substring of `str` starting at position `pos`.

    substring(str, pos [, len])

If `pos` is negative, the returned string will start `pos` characters from the end of `str`.

If `len` is omitted, the result is the substring from position `pos` until the end of `str`.

If `len` is provided, the substring of `str` `len` characters long starting at position `pos`
is returned. In case that `len` is smaller than 0, the result will be an empty string.

Examples:

    evql> SELECT substring("foobar", 2);
          -> "oobar"

    evql> SELECT substring("foobar", -3, 2)
          -> "ba"
