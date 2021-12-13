#pragma once

#ifndef GAL_LANG_GC_HPP
#define GAL_LANG_GC_HPP

namespace gal
{
	class object;

	struct gal_gc
	{
		static void add(object* obj);

		static void clear();
	};
}

#endif//GAL_LANG_GC_HPP
