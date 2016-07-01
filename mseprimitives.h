
// Copyright (c) 2015 Noah Lopez
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef MSEPRIMITIVES_H
#define MSEPRIMITIVES_H

#include <assert.h>
#include <limits>       // std::numeric_limits
#include <stdexcept>      // std::out_of_range

/*compiler specific defines*/
#ifdef _MSC_VER
#if (1700 > _MSC_VER)
#define MSVC2010_COMPATIBLE 1
#endif /*(1700 > _MSC_VER)*/
#if (1900 > _MSC_VER)
#define MSVC2013_COMPATIBLE 1
#endif /*(1900 > _MSC_VER)*/
#else /*_MSC_VER*/
#if (defined(__GNUC__) || defined(__GNUG__))
#define GPP_COMPATIBLE 1
#if (5 > __GNUC__)
#define GPP4P8_COMPATIBLE 1
#endif /*(5 > __GNUC__)*/
#endif /*(defined(__GNUC__) || defined(__GNUG__))*/
#endif /*_MSC_VER*/

#ifdef MSE_SAFER_SUBSTITUTES_DISABLED
#define MSE_PRIMITIVES_DISABLED
#define MSE_SAFERPTR_DISABLED
#endif /*MSE_SAFER_SUBSTITUTES_DISABLED*/

#if defined(MSVC2013_COMPATIBLE) || defined(MSVC2010_COMPATIBLE)
#define MSE_CONSTEXPR
#else // defined(MSVC2013_COMPATIBLE) || defined(MSVC2010_COMPATIBLE)
#define MSE_CONSTEXPR constexpr
#endif // defined(MSVC2013_COMPATIBLE) || defined(MSVC2010_COMPATIBLE)


namespace mse {

	/* This macro roughly simulates constructor inheritance. Originally it was used when some compilers didn't support
	constructor inheritance, but now we use it because of it's differences with standard constructor inheritance. */
#define MSE_USING(Derived, Base) \
    template<typename ...Args, typename = typename std::enable_if<std::is_constructible<Base, Args...>::value>::type> \
    Derived(Args &&...args) : Base(std::forward<Args>(args)...) {}


	/* When the mse primitive replacements are "disabled" they lose their default initialization and may cause problems for
	code that relies on it. */
#ifdef MSE_PRIMITIVES_DISABLED
	typedef bool CBool;
	typedef long long int CInt;
	typedef size_t CSize_t;
	static size_t as_a_size_t(CSize_t n) { return (n); }
#else /*MSE_PRIMITIVES_DISABLED*/

#ifndef NDEBUG
#ifndef MSE_SUPPRESS_CHECK_USE_BEFORE_SET
#define MSE_CHECK_USE_BEFORE_SET
#endif // !MSE_SUPPRESS_CHECK_USE_BEFORE_SET
#endif // !NDEBUG

	/* This class is just meant to act like the "bool" type, except that it has a default intialization value (false). */
	class CBool {
	public:
		// Constructs zero.
		CBool() : m_val(false) {}

		// Copy constructor
		CBool(const CBool &x) : m_val(x.m_val) { note_value_assignment(); };

		// Assignment operator
		CBool& operator=(const CBool &x) { note_value_assignment(); m_val = x.m_val; return (*this); }

		// Constructors from primitive boolean types
		CBool(bool   x) { note_value_assignment(); m_val = x; }

		// Casts to primitive boolean types
		operator bool() const { assert_initialized(); return m_val; }

		CBool& operator |=(const CBool &x) { assert_initialized(); m_val |= x.m_val; return (*this); }
		CBool& operator &=(const CBool &x) { assert_initialized(); m_val &= x.m_val; return (*this); }
		CBool& operator ^=(const CBool &x) { assert_initialized(); m_val ^= x.m_val; return (*this); }

		bool m_val;

#ifdef MSE_CHECK_USE_BEFORE_SET
		void note_value_assignment() { m_initialized = true; }
		void assert_initialized() const { assert(m_initialized); }
		bool m_initialized = false;
#else // MSE_CHECK_USE_BEFORE_SET
		void note_value_assignment() {}
		void assert_initialized() const {}
#endif // MSE_CHECK_USE_BEFORE_SET
	};


	template<typename _TDestination, typename _TSource>
	MSE_CONSTEXPR static bool sg_can_exceed_upper_bound() {
		return (
			((std::numeric_limits<_TSource>::is_signed == std::numeric_limits<_TDestination>::is_signed)
				&& (std::numeric_limits<_TSource>::digits > std::numeric_limits<_TDestination>::digits))
			|| ((std::numeric_limits<_TSource>::is_signed != std::numeric_limits<_TDestination>::is_signed)
				&& ((std::numeric_limits<_TSource>::is_signed && (std::numeric_limits<_TSource>::digits > (1 + std::numeric_limits<_TDestination>::digits)))
					|| ((!std::numeric_limits<_TSource>::is_signed) && ((1 + std::numeric_limits<_TSource>::digits) > std::numeric_limits<_TDestination>::digits))
					)
				)
			);
	}
	template<typename _TDestination, typename _TSource>
	MSE_CONSTEXPR static bool sg_can_exceed_lower_bound() {
		return (
			(std::numeric_limits<_TSource>::is_signed && (!std::numeric_limits<_TDestination>::is_signed))
			|| (std::numeric_limits<_TSource>::is_signed && (std::numeric_limits<_TSource>::digits > std::numeric_limits<_TDestination>::digits))
			);
	}

	template<typename _TDestination, typename _TSource>
	void g_assign_check_range(const _TSource &x) {
		/* This probably needs to be cleaned up. But at the moment it this should be mostly compile time complexity. And
		as is it avoids "signed/unsigned" mismatch warnings. */
		MSE_CONSTEXPR const bool rhs_can_exceed_upper_bound = sg_can_exceed_upper_bound<_TDestination, _TSource>();
		MSE_CONSTEXPR const bool rhs_can_exceed_lower_bound = sg_can_exceed_lower_bound<_TDestination, _TSource>();
		MSE_CONSTEXPR const bool can_exceed_bounds = rhs_can_exceed_upper_bound || rhs_can_exceed_lower_bound;
		if (can_exceed_bounds) {
			if (rhs_can_exceed_upper_bound) {
				if (x > _TSource(std::numeric_limits<_TDestination>::max())) {
					throw(std::out_of_range("out of range error - value to be assigned is out of range of the target (integer) type"));
				}
			}
			if (rhs_can_exceed_lower_bound) {
				/* We're assuming that std::numeric_limits<>::lowest() will never be greater than zero. */
				if (0 > x) {
					if (0 == std::numeric_limits<_TDestination>::lowest()) {
						throw(std::out_of_range("out of range error - value to be assigned is out of range of the target (integer) type"));
					}
					else if (x < _TSource(std::numeric_limits<_TDestination>::lowest())) {
						throw(std::out_of_range("out of range error - value to be assigned is out of range of the target (integer) type"));
					}
				}
			}
		}
	}

	/* The CInt and CSize_t classes are meant to substitute for standard "int" and "size_t" types. The differences between
	the standard types and these classes are that the classes have a default intialization value (zero), and the
	classes, as much as possible, try to prevent the problematic behaviour of (possibly negative) signed integers
	being cast (inadvertently) to the unsigned size_t type. For example, the expression (0 > (int)5 - (size_t)7) evaluates
	(unintuitively) to false, whereas the expression (0 > (CInt)5 - (CSize_t)7) evaluates to true. Also, the classes do
	some range checking. For example, the code "CSize_t s = -2;" will throw an exception. */
	template<typename _Ty>
	class TIntBase1 {
	public:
		// Constructs zero.
		TIntBase1() : m_val(0) {}

		// Copy constructor
		TIntBase1(const TIntBase1 &x) : m_val(x.m_val) { note_value_assignment(); };

		// Constructors from primitive integer types
		explicit TIntBase1(_Ty   x) { note_value_assignment(); m_val = x; }

		template<typename _Tz>
		void assign_check_range(const _Tz &x) {
			note_value_assignment();
			g_assign_check_range<_Ty, _Tz>(x);
		}

		_Ty m_val;

#ifdef MSE_CHECK_USE_BEFORE_SET
		void note_value_assignment() { m_initialized = true; }
		void assert_initialized() const { assert(m_initialized); }
		bool m_initialized = false;
#else // MSE_CHECK_USE_BEFORE_SET
		void note_value_assignment() {}
		void assert_initialized() const {}
#endif // MSE_CHECK_USE_BEFORE_SET
	};

#ifndef MSE_CINT_BASE_INTEGER_TYPE
#if SIZE_MAX <= ULONG_MAX
#define MSE_CINT_BASE_INTEGER_TYPE long int
#else // SIZE_MAX <= ULONG_MAX
#define MSE_CINT_BASE_INTEGER_TYPE long long int
#endif // SIZE_MAX <= ULONG_MAX
#endif // !MSE_CINT_BASE_INTEGER_TYPE

	class CInt : public TIntBase1<MSE_CINT_BASE_INTEGER_TYPE> {
	public:
		typedef MSE_CINT_BASE_INTEGER_TYPE _Ty;
		typedef TIntBase1<_Ty> _Myt;

		// Constructs zero.
		CInt() : _Myt() {}

		// Copy constructor
		CInt(const CInt &x) : _Myt(x) {};
		CInt(const _Myt &x) : _Myt(x) {};

		// Assignment operator
		CInt& operator=(const CInt &x) { (*this).note_value_assignment(); m_val = x.m_val; return (*this); }
		//CInt& operator=(const _Ty &x) { (*this).note_value_assignment(); m_val = x; return (*this); }

		CInt& operator=(long long x) { assign_check_range<long long>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CInt& operator=(long x) { assign_check_range<long>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CInt& operator=(int x) { assign_check_range<int>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CInt& operator=(short x) { assign_check_range<short>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CInt& operator=(char x) { assign_check_range<char>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CInt& operator=(size_t x) { assign_check_range<size_t>(x); m_val = static_cast<_Ty>(x); return (*this); }
		//CInt& operator=(CSize_t x) { assign_check_range<size_t>(x.as_a_size_t()); m_val = x.as_a_size_t(); return (*this); }
		/* We would have liked to have assignment operators for the unsigned primitive integer types, but one of them could
		potentially clash with the size_t assignment operator. */
		//CInt& operator=(unsigned long long x) { assign_check_range<unsigned long long>(x); m_val = static_cast<_Ty>(x); return (*this); }
		//CInt& operator=(unsigned long x) { assign_check_range<unsigned long>(x); m_val = static_cast<_Ty>(x); return (*this); }
		//CInt& operator=(unsigned int x) { assign_check_range<unsigned int>(x); m_val = static_cast<_Ty>(x); return (*this); }
		//CInt& operator=(unsigned short x) { assign_check_range<unsigned short>(x); m_val = static_cast<_Ty>(x); return (*this); }
		//CInt& operator=(unsigned char x) { assign_check_range<unsigned char>(x); m_val = static_cast<_Ty>(x); return (*this); }

		// Constructors from primitive integer types
		//CInt(_Ty   x) { m_val = x; }
		CInt(long long  x) { assign_check_range<long long>(x); m_val = static_cast<_Ty>(x); }
		CInt(long  x) { assign_check_range< long>(x); m_val = static_cast<_Ty>(x); }
		CInt(int   x) { assign_check_range<int>(x); m_val = static_cast<_Ty>(x); }
		CInt(short x) { assign_check_range<short>(x); m_val = static_cast<_Ty>(x); }
		CInt(char x) { assign_check_range<char>(x); m_val = static_cast<_Ty>(x); }
		CInt(size_t   x) { assign_check_range<size_t>(x); m_val = static_cast<_Ty>(x); }
		//CInt(CSize_t   x) { assign_check_range<size_t>(x.as_a_size_t()); m_val = x.as_a_size_t(); }
		/* We would have liked to have constructors for the unsigned primitive integer types, but one of them could
		potentially clash with the size_t constructor. */
		//CInt(unsigned long long  x) { assign_check_range<unsigned long long>(x); m_val = static_cast<_Ty>(x); }
		//CInt(unsigned long  x) { assign_check_range<unsigned long>(x); m_val = static_cast<_Ty>(x); }
		//CInt(unsigned int   x) { assign_check_range<unsigned int>(x); m_val = static_cast<_Ty>(x); }
		//CInt(unsigned short x) { assign_check_range<unsigned short>(x); m_val = static_cast<_Ty>(x); }
		//CInt(unsigned char x) { assign_check_range<unsigned char>(x); m_val = static_cast<_Ty>(x); }

		// Casts to primitive integer types
		operator _Ty() const { (*this).assert_initialized(); return m_val; }

		CInt operator ~() const { (*this).assert_initialized(); return CInt(~m_val); }
		CInt& operator |=(const CInt &x) { (*this).assert_initialized(); m_val |= x.m_val; return (*this); }
		CInt& operator &=(const CInt &x) { (*this).assert_initialized(); m_val &= x.m_val; return (*this); }
		CInt& operator ^=(const CInt &x) { (*this).assert_initialized(); m_val ^= x.m_val; return (*this); }

		CInt operator -() const { (*this).assert_initialized(); return CInt(-m_val); }
		CInt& operator +=(const CInt &x) { (*this).assert_initialized(); m_val += x.m_val; return (*this); }
		CInt& operator -=(const CInt &x) { (*this).assert_initialized();
			if (0 <= std::numeric_limits<_Ty>::lowest()) { (*this).assert_initialized();
				if (x.m_val > m_val) { (*this).assert_initialized(); /*check this*/
					throw(std::out_of_range("out of range error - value to be assigned is out of range of the target (integer) type"));
				}
			}
			m_val -= x.m_val; return (*this);
		}
		CInt& operator *=(const CInt &x) { (*this).assert_initialized(); m_val *= x.m_val; return (*this); }
		CInt& operator /=(const CInt &x) { (*this).assert_initialized(); m_val /= x.m_val; return (*this); }
		CInt& operator %=(const CInt &x) { (*this).assert_initialized(); m_val %= x.m_val; return (*this); }
		CInt& operator >>=(const CInt &x) { (*this).assert_initialized(); m_val >>= x.m_val; return (*this); }
		CInt& operator <<=(const CInt &x) { (*this).assert_initialized(); m_val <<= x.m_val; return (*this); }

		CInt operator +(const CInt &x) const { (*this).assert_initialized(); return CInt(m_val + x.m_val); }
		CInt operator +(long long x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CInt operator +(long x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CInt operator +(int x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CInt operator +(short x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CInt operator +(char x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CInt operator +(size_t x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		//CInt operator +(CSize_t x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }

		CInt operator -(const CInt &x) const { (*this).assert_initialized(); return CInt(m_val - x.m_val); }
		CInt operator -(long long x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(long x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(int x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(short x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(char x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(size_t x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		//CInt operator -(CSize_t x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }

		CInt operator *(const CInt &x) const { (*this).assert_initialized(); return CInt(m_val * x.m_val); }
		CInt operator *(long long x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CInt operator *(long x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CInt operator *(int x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CInt operator *(short x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CInt operator *(char x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CInt operator *(size_t x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		//CInt operator *(CSize_t x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }

		CInt operator /(const CInt &x) const { (*this).assert_initialized(); return CInt(m_val / x.m_val); }
		CInt operator /(long long x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CInt operator /(long x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CInt operator /(int x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CInt operator /(short x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CInt operator /(char x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CInt operator /(size_t x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		//CInt operator /(CSize_t x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }

		bool operator <(const CInt &x) const { (*this).assert_initialized(); return (m_val < x.m_val); }
		bool operator <(long long x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(long x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(int x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(short x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(char x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(size_t x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		//bool operator <(CSize_t x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }

		bool operator >(const CInt &x) const { (*this).assert_initialized(); return (m_val > x.m_val); }
		bool operator >(long long x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(long x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(int x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(short x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(char x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(size_t x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		//bool operator >(CSize_t x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }

		bool operator <=(const CInt &x) const { (*this).assert_initialized(); return (m_val <= x.m_val); }
		bool operator <=(long long x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(long x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(int x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(short x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(char x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(size_t x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		//bool operator <=(CSize_t x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }

		bool operator >=(const CInt &x) const { (*this).assert_initialized(); return (m_val >= x.m_val); }
		bool operator >=(long long x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(long x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(int x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(short x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(char x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(size_t x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		//bool operator >=(CSize_t x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }

		bool operator ==(const CInt &x) const { (*this).assert_initialized(); return (m_val == x.m_val); }
		bool operator ==(long long x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(long x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(int x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(short x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(char x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(size_t x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		//bool operator ==(CSize_t x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }

		bool operator !=(const CInt &x) const { (*this).assert_initialized(); return (m_val != x.m_val); }
		bool operator !=(long long x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(long x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(int x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(short x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(char x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(size_t x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		//bool operator !=(CSize_t x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }

		// INCREMENT/DECREMENT OPERATORS
		CInt& operator ++() { (*this).assert_initialized(); m_val++; return (*this); }
		CInt operator ++(int) { (*this).assert_initialized();
			CInt tmp(*this); // copy
			operator++(); // pre-increment
			return tmp;   // return old value
		}
		CInt& operator --() { (*this).assert_initialized();
			if (0 <= std::numeric_limits<_Ty>::lowest()) { (*this).assert_initialized();
				(*this) = (*this) - 1; return (*this);
			}
			else { (*this).assert_initialized();
				m_val--; return (*this);
			}
		}
		CInt operator --(int) { (*this).assert_initialized();
			CInt tmp(*this); // copy
			operator--(); // pre-decrement
			return tmp;   // return old value
		}

		//_Ty m_val;
	};

	class CSize_t;
	static size_t as_a_size_t(CSize_t n);

	/* Note that CSize_t does not have a default conversion to size_t. This is by design. Use the as_a_size_t() member
	function to get a size_t when necessary. */
	class CSize_t : public TIntBase1<size_t> {
	public:
		typedef size_t _Ty;
		typedef int _T_signed_primitive_integer_type;
		typedef TIntBase1<_Ty> _Myt;

		// Constructs zero.
		CSize_t() : _Myt() {}

		// Copy constructor
		CSize_t(const CSize_t &x) : _Myt(x) {};
		CSize_t(const _Myt &x) : _Myt(x) {};

		// Assignment operator
		CSize_t& operator=(const CSize_t &x) { m_val = x.m_val; return (*this); }
		//CSize_t& operator=(const _Ty &x) { m_val = x; return (*this); }

		CSize_t& operator=(long long x) { assign_check_range<long long>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CSize_t& operator=(long x) { assign_check_range<long>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CSize_t& operator=(int x) { assign_check_range<int>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CSize_t& operator=(short x) { assign_check_range<short>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CSize_t& operator=(char x) { assign_check_range<char>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CSize_t& operator=(size_t x) { assign_check_range<size_t>(x); m_val = static_cast<_Ty>(x); return (*this); }
		CSize_t& operator=(CInt x) { assign_check_range<MSE_CINT_BASE_INTEGER_TYPE>(x); m_val = static_cast<_Ty>(x); return (*this); }
		/* We would have liked to have assignment operators for the unsigned primitive integer types, but one of them could
		potentially clash with the size_t assignment operator. */
		//CSize_t& operator=(unsigned long long x) { assign_check_range<unsigned long long>(x); m_val = static_cast<_Ty>(x); return (*this); }
		//CSize_t& operator=(unsigned long x) { assign_check_range<unsigned long>(x); m_val = static_cast<_Ty>(x); return (*this); }
		//CSize_t& operator=(unsigned int x) { assign_check_range<unsigned int>(x); m_val = static_cast<_Ty>(x); return (*this); }
		//CSize_t& operator=(unsigned short x) { assign_check_range<unsigned short>(x); m_val = static_cast<_Ty>(x); return (*this); }
		//CSize_t& operator=(unsigned char x) { assign_check_range<unsigned char>(x); m_val = static_cast<_Ty>(x); return (*this); }

		// Constructors from primitive integer types
		//explicit CSize_t(_Ty   x) { m_val = x; }
		explicit CSize_t(long long  x) { assign_check_range<long long>(x); m_val = static_cast<_Ty>(x); }
		explicit CSize_t(long  x) { assign_check_range< long>(x); m_val = static_cast<_Ty>(x); }
		explicit CSize_t(int   x) { assign_check_range<int>(x); m_val = static_cast<_Ty>(x); }
		explicit CSize_t(short x) { assign_check_range<short>(x); m_val = static_cast<_Ty>(x); }
		explicit CSize_t(char x) { assign_check_range<char>(x); m_val = static_cast<_Ty>(x); }
		CSize_t(size_t   x) { assign_check_range<size_t>(x); m_val = static_cast<_Ty>(x); }
		/*explicit */CSize_t(CInt   x) { assign_check_range<MSE_CINT_BASE_INTEGER_TYPE>(x); m_val = static_cast<_Ty>(x); }
		/* We would have liked to have constructors for the unsigned primitive integer types, but one of them could
		potentially clash with the size_t constructor. */
		//explicit CSize_t(unsigned long long  x) { assign_check_range<unsigned long long>(x); m_val = static_cast<_Ty>(x); }
		//explicit CSize_t(unsigned long  x) { assign_check_range<unsigned long>(x); m_val = static_cast<_Ty>(x); }
		//explicit CSize_t(unsigned int   x) { assign_check_range<unsigned int>(x); m_val = static_cast<_Ty>(x); }
		//explicit CSize_t(unsigned short x) { assign_check_range<unsigned short>(x); m_val = static_cast<_Ty>(x); }
		//explicit CSize_t(unsigned char x) { assign_check_range<unsigned char>(x); m_val = static_cast<_Ty>(x); }

		// Casts to primitive integer types
		operator CInt() const { (*this).assert_initialized(); return CInt(m_val); }
#ifndef MSVC2010_COMPATIBLE
		explicit operator size_t() const { (*this).assert_initialized(); return (m_val); }
#endif /*MSVC2010_COMPATIBLE*/
		//size_t as_a_size_t() const { (*this).assert_initialized(); return m_val; }

		CSize_t operator ~() const { (*this).assert_initialized(); return (~m_val); }
		CSize_t& operator |=(const CSize_t &x) { (*this).assert_initialized(); m_val |= x.m_val; return (*this); }
		CSize_t& operator &=(const CSize_t &x) { (*this).assert_initialized(); m_val &= x.m_val; return (*this); }
		CSize_t& operator ^=(const CSize_t &x) { (*this).assert_initialized(); m_val ^= x.m_val; return (*this); }

		CInt operator -() const { (*this).assert_initialized(); /* Should unsigned types even support this opperator? */
			return (-(CInt(m_val)));
		}
		CSize_t& operator +=(const CSize_t &x) { (*this).assert_initialized(); m_val += x.m_val; return (*this); }
		CSize_t& operator -=(const CSize_t &x) { (*this).assert_initialized();
			if (0 <= std::numeric_limits<_Ty>::lowest()) { (*this).assert_initialized();
				if (x.m_val > m_val) { (*this).assert_initialized(); /*check this*/
					throw(std::out_of_range("out of range error - value to be assigned is out of range of the target (integer) type"));
				}
			}
			m_val -= x.m_val; return (*this);
		}
		CSize_t& operator *=(const CSize_t &x) { (*this).assert_initialized(); m_val *= x.m_val; return (*this); }
		CSize_t& operator /=(const CSize_t &x) { (*this).assert_initialized(); m_val /= x.m_val; return (*this); }
		CSize_t& operator %=(const CSize_t &x) { (*this).assert_initialized(); m_val %= x.m_val; return (*this); }
		CSize_t& operator >>=(const CSize_t &x) { (*this).assert_initialized(); m_val >>= x.m_val; return (*this); }
		CSize_t& operator <<=(const CSize_t &x) { (*this).assert_initialized(); m_val <<= x.m_val; return (*this); }

		CSize_t operator +(const CSize_t &x) const { (*this).assert_initialized(); return (m_val + x.m_val); }
		CInt operator +(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) + x); }
		CInt operator +(long long x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CInt operator +(long x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CInt operator +(int x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CInt operator +(short x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CInt operator +(char x) const { (*this).assert_initialized(); return ((*this) + CInt(x)); }
		CSize_t operator +(size_t x) const { (*this).assert_initialized(); return ((*this) + CSize_t(x)); }

		CInt operator -(const CSize_t &x) const { (*this).assert_initialized(); return (CInt(m_val) - CInt(x.m_val)); }
		CInt operator -(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) - x); }
		CInt operator -(long long x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(long x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(int x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(short x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(char x) const { (*this).assert_initialized(); return ((*this) - CInt(x)); }
		CInt operator -(size_t x) const { (*this).assert_initialized(); return ((*this) - CSize_t(x)); }

		CSize_t operator *(const CSize_t &x) const { (*this).assert_initialized(); return (m_val * x.m_val); }
		CInt operator *(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) * x); }
		CInt operator *(long long x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CInt operator *(long x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CInt operator *(int x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CInt operator *(short x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CInt operator *(char x) const { (*this).assert_initialized(); return ((*this) * CInt(x)); }
		CSize_t operator *(size_t x) const { (*this).assert_initialized(); return ((*this) * CSize_t(x)); }

		CSize_t operator /(const CSize_t &x) const { (*this).assert_initialized(); return (m_val / x.m_val); }
		CInt operator /(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) / x); }
		CInt operator /(long long x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CInt operator /(long x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CInt operator /(int x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CInt operator /(short x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CInt operator /(char x) const { (*this).assert_initialized(); return ((*this) / CInt(x)); }
		CSize_t operator /(size_t x) const { (*this).assert_initialized(); return ((*this) / CSize_t(x)); }

		bool operator <(const CSize_t &x) const { (*this).assert_initialized(); return (m_val < x.m_val); }
		bool operator <(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) < x); }
		bool operator <(long long x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(long x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(int x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(short x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(char x) const { (*this).assert_initialized(); return ((*this) < CInt(x)); }
		bool operator <(size_t x) const { (*this).assert_initialized(); return ((*this) < CSize_t(x)); }

		bool operator >(const CSize_t &x) const { (*this).assert_initialized(); return (m_val > x.m_val); }
		bool operator >(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) > x); }
		bool operator >(long long x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(long x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(int x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(short x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(char x) const { (*this).assert_initialized(); return ((*this) > CInt(x)); }
		bool operator >(size_t x) const { (*this).assert_initialized(); return ((*this) > CSize_t(x)); }

		bool operator <=(const CSize_t &x) const { (*this).assert_initialized(); return (m_val <= x.m_val); }
		bool operator <=(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) <= x); }
		bool operator <=(long long x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(long x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(int x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(short x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(char x) const { (*this).assert_initialized(); return ((*this) <= CInt(x)); }
		bool operator <=(size_t x) const { (*this).assert_initialized(); return ((*this) <= CSize_t(x)); }

		bool operator >=(const CSize_t &x) const { (*this).assert_initialized(); return (m_val >= x.m_val); }
		bool operator >=(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) >= x); }
		bool operator >=(long long x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(long x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(int x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(short x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(char x) const { (*this).assert_initialized(); return ((*this) >= CInt(x)); }
		bool operator >=(size_t x) const { (*this).assert_initialized(); return ((*this) >= CSize_t(x)); }

		bool operator ==(const CSize_t &x) const { (*this).assert_initialized(); return (m_val == x.m_val); }
		bool operator ==(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) == x); }
		bool operator ==(long long x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(long x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(int x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(short x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(char x) const { (*this).assert_initialized(); return ((*this) == CInt(x)); }
		bool operator ==(size_t x) const { (*this).assert_initialized(); return ((*this) == CSize_t(x)); }

		bool operator !=(const CSize_t &x) const { (*this).assert_initialized(); return (m_val != x.m_val); }
		bool operator !=(const CInt &x) const { (*this).assert_initialized(); return (CInt(m_val) != x); }
		bool operator !=(long long x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(long x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(int x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(short x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(char x) const { (*this).assert_initialized(); return ((*this) != CInt(x)); }
		bool operator !=(size_t x) const { (*this).assert_initialized(); return ((*this) != CSize_t(x)); }

		// INCREMENT/DECREMENT OPERATORS
		CSize_t& operator ++() { (*this).assert_initialized(); m_val++; return (*this); }
		CSize_t operator ++(int) { (*this).assert_initialized();
			CSize_t tmp(*this); // copy
			operator++(); // pre-increment
			return tmp;   // return old value
		}
		CSize_t& operator --() { (*this).assert_initialized();
			if (0 <= std::numeric_limits<_Ty>::lowest()) { (*this).assert_initialized();
				(*this) = (*this) - 1; return (*this);
			}
			else { (*this).assert_initialized();
				m_val--; return (*this);
			}
		}
		CSize_t operator --(int) { (*this).assert_initialized();
			CSize_t tmp(*this); // copy
			operator--(); // pre-decrement
			return tmp;   // return old value
		}

		//_Ty m_val;

		friend size_t as_a_size_t(CSize_t n);
	};
	size_t as_a_size_t(CSize_t n) { n.assert_initialized(); return n.m_val; }

	inline CInt operator+(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); rhs.assert_initialized(); return CSize_t(lhs) + rhs; }
	inline CSize_t operator+(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) + rhs; }
	inline CInt operator+(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) + rhs; }
	inline CInt operator+(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) + as_a_size_t(rhs); }
	inline CInt operator+(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs + as_a_size_t(rhs); }
	inline CInt operator-(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); return CSize_t(lhs) - rhs; }
	inline CInt operator-(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) - rhs; }
	inline CInt operator-(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) - rhs; }
	inline CInt operator-(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) - as_a_size_t(rhs); }
	inline CInt operator-(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs - as_a_size_t(rhs); }
	inline CInt operator*(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); return CSize_t(lhs) * rhs; }
	inline CSize_t operator*(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) * rhs; }
	inline CInt operator*(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) * rhs; }
	inline CInt operator*(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) * as_a_size_t(rhs); }
	inline CInt operator*(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs * as_a_size_t(rhs); }
	inline CInt operator/(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); return CSize_t(lhs) / rhs; }
	inline CSize_t operator/(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) / rhs; }
	inline CInt operator/(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) / rhs; }
	inline CInt operator/(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) / as_a_size_t(rhs); }
	inline CInt operator/(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs / as_a_size_t(rhs); }

	inline bool operator<(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); return CSize_t(lhs) < rhs; }
	inline bool operator<(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) < rhs; }
	inline bool operator<(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) < rhs; }
	inline bool operator<(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) < as_a_size_t(rhs); }
	inline bool operator<(long long lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) < rhs; }
	inline bool operator<(long long lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) < as_a_size_t(rhs); }
	inline bool operator<(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs < as_a_size_t(rhs); }
	inline bool operator>(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); return CSize_t(lhs) > rhs; }
	inline bool operator>(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) > rhs; }
	inline bool operator>(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) > rhs; }
	inline bool operator>(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) > as_a_size_t(rhs); }
	inline bool operator>(long long lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) > rhs; }
	inline bool operator>(long long lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) > as_a_size_t(rhs); }
	inline bool operator>(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs > as_a_size_t(rhs); }
	inline bool operator<=(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); return CSize_t(lhs) <= rhs; }
	inline bool operator<=(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) <= rhs; }
	inline bool operator<=(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) <= rhs; }
	inline bool operator<=(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) <= as_a_size_t(rhs); }
	inline bool operator<=(long long lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) <= rhs; }
	inline bool operator<=(long long lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) <= as_a_size_t(rhs); }
	inline bool operator<=(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs <= as_a_size_t(rhs); }
	inline bool operator>=(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); return CSize_t(lhs) >= rhs; }
	inline bool operator>=(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) >= rhs; }
	inline bool operator>=(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) >= rhs; }
	inline bool operator>=(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) >= as_a_size_t(rhs); }
	inline bool operator>=(long long lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) >= rhs; }
	inline bool operator>=(long long lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) >= as_a_size_t(rhs); }
	inline bool operator>=(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs >= as_a_size_t(rhs); }
	inline bool operator==(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); return CSize_t(lhs) == rhs; }
	inline bool operator==(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) == rhs; }
	inline bool operator==(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) == rhs; }
	inline bool operator==(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) == as_a_size_t(rhs); }
	inline bool operator==(long long lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) == rhs; }
	inline bool operator==(long long lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) == as_a_size_t(rhs); }
	inline bool operator==(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs == as_a_size_t(rhs); }
	inline bool operator!=(size_t lhs, const CInt &rhs) { rhs.assert_initialized(); return CSize_t(lhs) != rhs; }
	inline bool operator!=(size_t lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CSize_t(lhs) != rhs; }
	inline bool operator!=(int lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) != rhs; }
	inline bool operator!=(int lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) != as_a_size_t(rhs); }
	inline bool operator!=(long long lhs, const CInt &rhs) { rhs.assert_initialized(); return CInt(lhs) != rhs; }
	inline bool operator!=(long long lhs, const CSize_t &rhs) { rhs.assert_initialized(); return CInt(lhs) != as_a_size_t(rhs); }
	inline bool operator!=(const CInt &lhs, const CSize_t &rhs) { rhs.assert_initialized(); return lhs != as_a_size_t(rhs); }
#endif /*MSE_PRIMITIVES_DISABLED*/

	static void s_type_test1() {
#ifdef MSE_SELF_TESTS
		CInt i1(3);
		CInt i2 = 5;
		CInt i3;
		i3 = 7;
		CInt i4 = i1 + i2;
		i4 = i1 + 17;
		i4 = 19 + i1;
		i4 += i2;
		i4 -= 23;
		i4++;
		CBool b1 = (i1 < i2);
		b1 = (i1 < 17);
		b1 = (19 < i1);
		b1 = (i1 == i2);
		b1 = (i1 == 17);
		b1 = (19 == i1);

		CSize_t szt1(3);
		CSize_t szt2 = 5;
		CSize_t szt3;
		szt3 = 7;
		CSize_t szt4 = szt1 + szt2;
		szt4 = szt1 + 17;
		szt4 = 19 + szt1;
		CInt i11 = 19 + szt1;
		szt4 += szt2;
		szt4 -= 23;
		szt4++;
#ifndef MSVC2010_COMPATIBLE
		size_t szt5 = (size_t)szt4;
#endif /*MSVC2010_COMPATIBLE*/
		bool b3 = (szt1 < szt2);
		b3 = (szt1 < 17);
		b3 = (19 < szt1);
		CBool b2 = (19 < szt1);
		b3 = (szt1 == szt2);
		b3 = (szt1 == 17);
		b3 = (19 == szt1);
		CBool b4 = (b1 < b2);
		b4 = (b1 == b2);
		b4 = (b1 > b3);
		b4 = (b3 >= b1);
		b4 = (b3 == b1);
		b4 = (b1 && b2);
		b4 = (b1 || b3);
		b4 = (b3 && b1);
		b4 |= b1;
		b4 &= b3;
#endif // MSE_SELF_TESTS
	}

#ifdef MSE_SAFERPTR_DISABLED
	template<typename _Ty>
	using TSaferPtr = _Ty*;

	template<typename _Ty>
	using TSaferPtrForLegacy = _Ty*;
#else /*MSE_SAFERPTR_DISABLED*/

	class CSaferPtrBase {
	public:
		/* setToNull() needs to be available even when the smart pointer is const, because the object it points to may become
		invalid (deleted). */
		virtual void setToNull() const = 0;
	};

#ifndef NDEBUG
#ifndef MSE_SUPPRESS_TSAFERPTR_CHECK_USE_BEFORE_SET
#define MSE_TSAFERPTR_CHECK_USE_BEFORE_SET
#endif // !MSE_SUPPRESS_TSAFERPTR_CHECK_USE_BEFORE_SET
#endif // !NDEBUG

	/* TSaferPtr behaves similar to, and is largely compatible with, native pointers. It's a bit safer in that it initializes to
	nullptr by default and checks for attempted dereference of null pointers. */
	template<typename _Ty>
	class TSaferPtr : public CSaferPtrBase {
	public:
		TSaferPtr() : m_ptr(nullptr) {}
		TSaferPtr(_Ty* ptr) : m_ptr(ptr) { note_value_assignment(); }
		TSaferPtr(const TSaferPtr<_Ty>& src) : m_ptr(src.m_ptr) { note_value_assignment(); }
		template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
		TSaferPtr(const TSaferPtr<_Ty2>& src_cref) : m_ptr(src_cref.m_ptr) { note_value_assignment(); }
		virtual ~TSaferPtr() {}

		virtual void setToNull() const { m_ptr = nullptr; }

		void raw_pointer(_Ty* ptr) { note_value_assignment(); m_ptr = ptr; }
		_Ty* raw_pointer() const { return m_ptr; }
		_Ty* get() const { return m_ptr; }
		_Ty& operator*() const {
			assert_initialized();
#ifndef MSE_DISABLE_TSAFERPTR_CHECKS
			if (nullptr == m_ptr) {
				throw(std::out_of_range("attempt to dereference null pointer - mse::TSaferPtr"));
			}
#endif /*MSE_DISABLE_TSAFERPTR_CHECKS*/
			return (*m_ptr);
		}
		_Ty* operator->() const {
			assert_initialized();
#ifndef MSE_DISABLE_TSAFERPTR_CHECKS
			if (nullptr == m_ptr) {
				throw(std::out_of_range("attempt to dereference null pointer - mse::TSaferPtr"));
			}
#endif /*MSE_DISABLE_TSAFERPTR_CHECKS*/
			return m_ptr;
		}
		TSaferPtr<_Ty>& operator=(_Ty* ptr) {
			note_value_assignment();
			m_ptr = ptr;
			return (*this);
		}
		TSaferPtr<_Ty>& operator=(const TSaferPtr<_Ty>& _Right_cref) {
			note_value_assignment();
			m_ptr = _Right_cref.m_ptr;
			return (*this);
		}
		bool operator==(const _Ty* _Right_cref) const { assert_initialized(); return (_Right_cref == m_ptr); }
		bool operator!=(const _Ty* _Right_cref) const { assert_initialized(); return (!((*this) == _Right_cref)); }
		bool operator==(const TSaferPtr<_Ty> &_Right_cref) const { assert_initialized(); return (_Right_cref == m_ptr); }
		bool operator!=(const TSaferPtr<_Ty> &_Right_cref) const { assert_initialized(); return (!((*this) == _Right_cref)); }

		bool operator!() const { assert_initialized(); return (!m_ptr); }
		operator bool() const {
			assert_initialized();
			return (m_ptr != nullptr);
		}

		explicit operator _Ty*() const {
			assert_initialized();
			if (nullptr == m_ptr) {
				int q = 3; /* just a line of code for putting a debugger break point */
			}
			return m_ptr;
		}

		/* m_ptr needs to be mutable so that it can be set to nullptr when the object it points to is no longer valid (i.e. has
		been deleted) even in cases when this smart pointer is const. */
		mutable _Ty* m_ptr;

#ifdef MSE_TSAFERPTR_CHECK_USE_BEFORE_SET
		void note_value_assignment() { m_initialized = true; }
		void assert_initialized() const { assert(m_initialized); }
		bool m_initialized = false;
#else // MSE_TSAFERPTR_CHECK_USE_BEFORE_SET
		void note_value_assignment() {}
		void assert_initialized() const {}
#endif // MSE_TSAFERPTR_CHECK_USE_BEFORE_SET
	};

	/* TSaferPtrForLegacy is similar to TSaferPtr, but more readily converts to a native pointer implicitly. So when replacing
	native pointers with safer pointers in legacy code, fewer code changes (explicit casts) may be required when using this
	template. */
	template<typename _Ty>
	class TSaferPtrForLegacy : public CSaferPtrBase {
	public:
		TSaferPtrForLegacy() : m_ptr(nullptr) {}
		TSaferPtrForLegacy(_Ty* ptr) : m_ptr(ptr) { note_value_assignment(); }
		template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
		TSaferPtrForLegacy(const TSaferPtrForLegacy<_Ty2>& src_cref) : m_ptr(src_cref.m_ptr) { note_value_assignment(); }
		virtual ~TSaferPtrForLegacy() {}

		virtual void setToNull() const { m_ptr = nullptr; }

		void raw_pointer(_Ty* ptr) { note_value_assignment(); m_ptr = ptr; }
		_Ty* raw_pointer() const { return m_ptr; }
		_Ty* get() const { return m_ptr; }
		_Ty& operator*() const {
			assert_initialized();
#ifndef MSE_DISABLE_TSAFERPTRFORLEGACY_CHECKS
			if (nullptr == m_ptr) {
				throw(std::out_of_range("attempt to dereference null pointer - mse::TSaferPtrForLegacy"));
			}
#endif /*MSE_DISABLE_TSAFERPTRFORLEGACY_CHECKS*/
			return (*m_ptr);
		}
		_Ty* operator->() const {
			assert_initialized();
#ifndef MSE_DISABLE_TSAFERPTRFORLEGACY_CHECKS
			if (nullptr == m_ptr) {
				throw(std::out_of_range("attempt to dereference null pointer - mse::TSaferPtrForLegacy"));
			}
#endif /*MSE_DISABLE_TSAFERPTRFORLEGACY_CHECKS*/
			return m_ptr;
		}
		TSaferPtrForLegacy<_Ty>& operator=(_Ty* ptr) {
			note_value_assignment();
			m_ptr = ptr;
			return (*this);
		}
		//operator bool() const { return m_ptr; }

		operator _Ty*() const {
			assert_initialized();
			if (nullptr == m_ptr) {
				int q = 3; /* just a line of code for putting a debugger break point */
			}
			return m_ptr;
		}

		/* m_ptr needs to be mutable so that it can be set to nullptr when the object it points to is no longer valid (i.e. has
		been deleted) even in cases when this smart pointer is const. */
		mutable _Ty* m_ptr;

#ifdef MSE_TSAFERPTR_CHECK_USE_BEFORE_SET
		void note_value_assignment() { m_initialized = true; }
		void assert_initialized() const { assert(m_initialized); }
		bool m_initialized = false;
#else // MSE_TSAFERPTR_CHECK_USE_BEFORE_SET
		void note_value_assignment() {}
		void assert_initialized() const {}
#endif // MSE_TSAFERPTR_CHECK_USE_BEFORE_SET
	};
#endif /*MSE_SAFERPTR_DISABLED*/

}

#endif /*ndef MSEPRIMITIVES_H*/
