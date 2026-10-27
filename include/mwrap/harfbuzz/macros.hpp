// <mwrap/harfbuzz/macros.hpp> — HarfBuzz's macros as macros, for the ones a
// function cannot be: HB_UNTAG (it expands to four arguments, for a
// printf-style call) and HB_VERSION_ATLEAST in a preprocessor test. Include it
// before `import mcpplibs.mwrap.harfbuzz;`: it is <hb.h>, <hb-ot.h> and
// <hb-aat.h>, so every other macro is defined too, and while a macro is
// defined it expands even inside `mcpplibs::mwrap::harfbuzz::macros::NAME`.
#pragma once
#include <hb.h>
#include <hb-ot.h>
#include <hb-aat.h>
