## Tracking ID Encoding

the tracking id is just a different representation of the namespace key. the
general algorithm to convert a namespace key into a tracking id is:

  - escape the namespace string using the escaping rules below
  - split the encoded namespace string into 12-byte chunks
  - for each chunk:
    - intepret the 12-bytes chunk as a base36 encoded 64bit unsigned integer
    - xor this integer with 0xB16B00B5B16B00B5
    - build the base34 string representation of xor-ed value
    - if the base34 string is shorter than 13 bytes and this is not the last
      chunk, pad the base34 string to 13 bytes using the `z` character as padding.
  - concatenate the (padded) base34 strings
  - the concatenated string is the tracking id

### Examples:
 
  - `dawanda` => `5C4RPVX58H04B`
  - `zscale-production` => `78KJCXEC8Q8TU5C4RPW6WV8U98`
  - `000000-production` => `48CF7E4RUKXUX48CF7E4QS7HI85C4RPW6WV8U98`

### Escaping Rules

to make this scheme work the namespace key must be a valid base36 value. since
the allowed chars in a namespace key are `A-Z0-9_-` and the namespace key is case
insensitive we must fit a 37 char alphabet into the 36 bits that base36 gives us
per point. So we must escape some values before encoding.

we use `x` as the escaping character. a single `x` character denotes the start
of an escaped sequence. the `x` character must be followed by exactly one byte.
these are the valid escape sequences:

  - `xx`  =>  a literal `x`
  - `xa`  =>  an underscore `_`
  - `xb`  =>  a hyphen `-`
  - `x0`  =>  a literal zero `0`

### Ruby Implementation

    def ztrackid_encode(str)
      str.gsub!("0", "x0")
      str.gsub!("x", "xx")
      str.gsub!("_", "xa")
      str.gsub!("-", "xb")

      out = ""

      num_chunks = ((str.length + (12 - 1)) / 12)
      num_chunks.times do |i|
        val = str[(i*12)..(i*12+11)].to_i(36)
        val ^= 0xB16B00B5B16B00B5
        chunk = val.to_s(34)

        if (i < num_chunks - 1)
          chunk += "z" * (13 - chunk.length)
        end

        out << chunk
      end

      out.upcase
    end

    ztrackid_encode("zscale-production")

### C++ Implementation

    String ztrackid_decode(const String& encoded) {
      static const char base36[37] = "0123456789abcdefghijklmnopqrstuvwxyz";

      auto decoded = (char*) alloca(encoded.size());
      if (!decoded) {
        RAISE(kMallocError, "alloca() failed");
      }

      auto begin = encoded.data();
      auto len = encoded.size();
      auto end = begin + len;
      auto out = decoded + len;
      char lastchr = 0;
      for (auto cur = begin + len - 1 - (len - 1) % 13; cur >= begin;
          end = cur, cur -= 13) {
        uint64_t v = 0;

        for (auto chr = cur; chr < end; ++chr) {
          auto c = *chr;
          if (c >= '0' && c <= '9') {
            c -= '0';
          } else if (c >= 'A' && c <= 'X') {
            c -= 'A' - 10;
          } else if (c >= 'a' && c <= 'x') {
            c -= 'a' - 10;
          } else {
            break;
          }

          v *= 34;
          v += c;
        }

        v ^= 0xB16B00B5B16B00B5;

        do {
          auto chr = base36[v % 36];

          if (chr == 'x') {
            switch (lastchr) {

              case 'a':
                *out = '_';
                continue;

              case 'b':
                *out = '-';
                continue;

              case 'x':
              case '0':
                continue;

            }
          }

          *(--out) = lastchr = chr;
        } while (v /= 36);
      }

      return String(out, encoded.size() - (out - decoded));
    }


