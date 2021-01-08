#pragma once

// ALIAS DEFINES //
// These are used to allow replacement later without changing code base


template<typename T>
constexpr auto BIT(T x) { return 1 << x; }

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

#ifdef VEL_DEBUG
#define VEL_ASSERT(x, ...) { if(!(x)) { VEL_CLIENT_ERROR("Assertion Failed: {0}", x); __debugbreak(); } }
#define VEL_CORE_ASSERT(x, ...) { if(!(x)) { VEL_CORE_ERROR("Assertion Failed: {0}", x); __debugbreak(); } }
#else
#define VEL_ASSERT(x,...)
#define VEL_CORE_ASSERT(x,...)
#endif