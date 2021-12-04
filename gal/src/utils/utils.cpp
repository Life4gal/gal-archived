#include <utils/assert.hpp>
#include <utils/utils.hpp>
#include <vm/common.hpp>

namespace gal
{
	int utf8_encode_num_bytes(int value)
	{
		gal_assert(value >= 0, "Cannot encode a negative value.");

		if (value <= 0x7f) return 1;
		if (value <= 0x7ff) return 2;
		if (value <= 0xffff) return 3;
		if (value <= 0x10ffff) return 4;
		return 0;
	}

	int utf8_encode(int value, std::uint8_t* bytes)
	{
		if (value <= 0x7f)
		{
			// Single byte (i.e. fits in ASCII).
			*bytes = value & 0x7f;
			return 1;
		}
		else if (value <= 0x7ff)
		{
			// Two byte sequence: 110xxxxx 10xxxxxx.
			*bytes = 0xc0 | ((value & 0x7c0) >> 6);
			++bytes;
			*bytes = 0x80 | (value & 0x3f);
			return 2;
		}
		else if (value <= 0xffff)
		{
			// Three byte sequence: 1110xxxx 10xxxxxx 10xxxxxx.
			*bytes = 0xe0 | ((value & 0xf000) >> 12);
			++bytes;
			*bytes = 0x80 | ((value & 0xfc0) >> 6);
			++bytes;
			*bytes = 0x80 | (value & 0x3f);
			return 3;
		}
		else if (value <= 0x10ffff)
		{
			// Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
			*bytes = 0xf0 | ((value & 0x1c0000) >> 18);
			++bytes;
			*bytes = 0x80 | ((value & 0x3f000) >> 12);
			++bytes;
			*bytes = 0x80 | ((value & 0xfc0) >> 6);
			++bytes;
			*bytes = 0x80 | (value & 0x3f);
			return 4;
		}

		// Invalid Unicode value. See: http://tools.ietf.org/html/rfc3629
		UNREACHABLE();
		return 0;
	}

	int utf8_decode(const std::uint8_t* bytes, gal_size_type length)
	{
		// Single byte (i.e. fits in ASCII).
		if (*bytes <= 0x7f) return *bytes;

		int				 value;
		decltype(length) remaining_bytes;
		if ((*bytes & 0xe0) == 0xc0)
		{
			// Two byte sequence: 110xxxxx 10xxxxxx.
			value			= *bytes & 0x1f;
			remaining_bytes = 1;
		}
		else if ((*bytes & 0xf0) == 0xe0)
		{
			// Three byte sequence: 1110xxxx	 10xxxxxx 10xxxxxx.
			value			= *bytes & 0x0f;
			remaining_bytes = 2;
		}
		else if ((*bytes & 0xf8) == 0xf0)
		{
			// Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
			value			= *bytes & 0x07;
			remaining_bytes = 3;
		}
		else
		{
			// Invalid UTF-8 sequence.
			return -1;
		}

		// Don't read past the end of the buffer on truncated UTF-8.
		if (remaining_bytes > length - 1) return -1;

		while (remaining_bytes > 0)
		{
			++bytes;
			--remaining_bytes;

			// Remaining bytes must be of form 10xxxxxx.
			if ((*bytes & 0xc0) != 0x80) return -1;

			value = value << 6 | (*bytes & 0x3f);
		}

		return value;
	}

	int utf8_decode_num_bytes(const std::uint8_t byte)
	{
		// If the byte starts with 10xxxxx, it's the middle of a UTF-8 sequence, so
		// don't count it at all.
		if ((byte & 0xc0) == 0x80) return 0;

		// The first byte's high bits tell us how many bytes are in the UTF-8
		// sequence.
		if ((byte & 0xf8) == 0xf0) return 4;
		if ((byte & 0xf0) == 0xe0) return 3;
		if ((byte & 0xe0) == 0xc0) return 2;
		return 1;
	}

	gal_size_type bit_ceil(gal_size_type n)
	{
		--n;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		++n;

		return n;
	}
}// namespace gal
