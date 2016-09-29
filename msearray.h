
// Copyright (c) 2015 Noah Lopez
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef MSEMSEARRAY_H
#define MSEMSEARRAY_H

//define MSE_MSEARRAY_USE_MSE_PRIMITIVES 1
#ifdef MSE_MSEARRAY_USE_MSE_PRIMITIVES
#include "mseprimitives.h"
#endif // MSE_MSEARRAY_USE_MSE_PRIMITIVES

#include <array>
#include <assert.h>
#include <memory>
#include <unordered_map>
#include <functional>
#include <tuple>
#ifdef MSE_SELF_TESTS
#include <iostream>
#include <string>
#endif // MSE_SELF_TESTS


namespace mse {

#ifdef MSE_MSEARRAY_USE_MSE_PRIMITIVES
	typedef mse::CSize_t msear_size_t;
	typedef mse::CInt msear_int;
	typedef bool msear_bool; // no added safety benefit to using mse::CBool in this case
	#define msear_as_a_size_t as_a_size_t
#else // MSE_MSEARRAY_USE_MSE_PRIMITIVES
	typedef size_t msear_size_t;
	typedef long long int msear_int;
	typedef bool msear_bool;
	typedef size_t msear_as_a_size_t;
#endif // MSE_MSEARRAY_USE_MSE_PRIMITIVES

	/* msear_pointer behaves similar to native pointers. It's a bit safer in that it initializes to
	nullptr by default and checks for attempted dereference of null pointers. */
	template<typename _Ty>
	class msear_pointer {
	public:
		msear_pointer() : m_ptr(nullptr) {}
		msear_pointer(_Ty* ptr) : m_ptr(ptr) {}
		msear_pointer(const msear_pointer<_Ty>& src) : m_ptr(src.m_ptr) {}

		_Ty& operator*() const {
#ifndef MSE_DISABLE_MSEAR_POINTER_CHECKS
			if (nullptr == m_ptr) { throw(std::out_of_range("attempt to dereference null pointer - mse::msear_pointer")); }
#endif /*MSE_DISABLE_MSEAR_POINTER_CHECKS*/
			return (*m_ptr);
		}
		_Ty* operator->() const {
#ifndef MSE_DISABLE_MSEAR_POINTER_CHECKS
			if (nullptr == m_ptr) { throw(std::out_of_range("attempt to dereference null pointer - mse::msear_pointer")); }
#endif /*MSE_DISABLE_MSEAR_POINTER_CHECKS*/
			return m_ptr;
		}
		msear_pointer<_Ty>& operator=(_Ty* ptr) {
			m_ptr = ptr;
			return (*this);
		}
		bool operator==(const msear_pointer _Right_cref) const { return (_Right_cref.m_ptr == m_ptr); }
		bool operator!=(const msear_pointer _Right_cref) const { return (!((*this) == _Right_cref)); }
		bool operator==(const _Ty* _Right_cref) const { return (_Right_cref == m_ptr); }
		bool operator!=(const _Ty* _Right_cref) const { return (!((*this) == _Right_cref)); }

		bool operator!() const { return (!m_ptr); }
		operator bool() const { return (m_ptr != nullptr); }

		operator _Ty*() const { return m_ptr; }

		_Ty* m_ptr;
	};

#ifndef _XSTD
#define _XSTD ::std::
#endif /*_XSTD*/

	/* Note that, at the moment, msearray inherits publicly from std::array. This is not intended to be a permanent
		characteristic of msearray and any reference to, or interpretation of, an msearray as an std::array is (and has
		always been) depricated. msearray endeavors to support the subset of the std::array interface that is compatible
		with the security/safety goals of msearray. (The remaining part of the std::array interface may be supported, as a
		user option, for compatibility.)
		In particular, keep in mind that std::array does not have a virtual destructor, so deallocating an msearray as an
		std::array would result in memory leaks. */
	template<class _Ty, size_t _Size>
	class msearray : public std::array<_Ty, _Size> {
	public:
		typedef std::array<_Ty, _Size> base_class;
		typedef msearray<_Ty, _Size> _Myt;

		msearray() {}
		msearray(const msearray& src) : base_class(src) {}
		msearray(const std::array<_Ty, _Size>& src) : base_class(src) {}
		msearray(_XSTD initializer_list<typename base_class::value_type> _Ilist) {
			/* std::array<> is an "aggregate type" (basically a POD struct with no base class, constructors or private
			data members (details here: http://en.cppreference.com/w/cpp/language/aggregate_initialization)). As such,
			support for construction from initializer list is automatically generated by the compiler. But since
			msearray has a base class (or at least I think that's why), it doesn't qualify to have support for
			construction from initializer list automatically generated by the compiler. So we have to support it here
			manually. But according to the referenced webpage, it seems that having a public base class will no longer
			by a disqualifying criteria in C++17. The benefit of having the compiler automatically generate support for
			initializer lists is that the initialization is more likely to occur at compile-time. Apparently. So for
			C++17 compliant compilers we should probably lose all these constructors. */
			assert(_Size >= _Ilist.size());
			msear_size_t count = 0;
			auto Il_it = _Ilist.begin();
			auto target_it = (*this).begin();
			for (; _Ilist.end() != Il_it, count < _Size; Il_it++, count+=1, target_it++) {
				(*target_it) = (*Il_it);
			}
			static const auto default_Ty = _Ty();
			for (; count < _Size; count+=1, target_it++) {
				(*target_it) = default_Ty;
			}
		}

		typename base_class::const_reference operator[](size_t _P) const {
			return (*this).at(_P);
		}
		typename base_class::reference operator[](size_t _P) {
			return (*this).at(_P);
		}
		typename base_class::reference front() {	// return first element of mutable sequence
			if (0 == (*this).size()) { throw(std::out_of_range("front() on empty - typename base_class::reference front() - msearray")); }
			return base_class::front();
		}
		typename base_class::const_reference front() const {	// return first element of nonmutable sequence
			if (0 == (*this).size()) { throw(std::out_of_range("front() on empty - typename base_class::const_reference front() - msearray")); }
			return base_class::front();
		}
		typename base_class::reference back() {	// return last element of mutable sequence
			if (0 == (*this).size()) { throw(std::out_of_range("back() on empty - typename base_class::reference back() - msearray")); }
			return base_class::back();
		}
		typename base_class::const_reference back() const {	// return last element of nonmutable sequence
			if (0 == (*this).size()) { throw(std::out_of_range("back() on empty - typename base_class::const_reference back() - msearray")); }
			return base_class::back();
		}


		/* Note that, at the moment, ss_const_iterator_type inherits publicly from base_class::const_iterator. This is not intended to be a permanent
		characteristc of ss_const_iterator_type and any reference to, or interpretation of, an ss_const_iterator_type as an base_class::const_iterator is (and has
		always been) depricated. ss_const_iterator_type endeavors to support (and continue to support) the subset of the base_class::const_iterator
		interface that is compatible with the security/safety goals of ss_const_iterator_type.
		In particular, keep in mind that base_class::const_iterator does not have a virtual destructor, so deallocating an ss_const_iterator_type as an
		base_class::const_iterator would result in memory leaks. */
		class ss_const_iterator_type : public base_class::const_iterator {
		public:
			typedef typename base_class::const_iterator::iterator_category iterator_category;
			typedef typename base_class::const_iterator::value_type value_type;
			typedef typename base_class::const_iterator::difference_type difference_type;
			typedef difference_type distance_type;	// retained
			typedef typename base_class::const_iterator::pointer pointer;
			typedef typename base_class::const_pointer const_pointer;
			typedef typename base_class::const_iterator::reference reference;
			typedef typename base_class::const_reference const_reference;

			ss_const_iterator_type() {}
			void reset() { set_to_end_marker(); }
			bool points_to_an_item() const {
				if ((1 <= m_owner_cptr->size()) && (m_index < m_owner_cptr->size())) { return true; }
				else {
					if (m_index == m_owner_cptr->size()) { return false; }
					else { throw(std::out_of_range("attempt to use invalid ss_const_iterator_type - bool points_to_an_item() const - ss_const_iterator_type - msearray")); }
				}
			}
			bool points_to_end_marker() const {
				if (false == points_to_an_item()) {
					if (m_index == m_owner_cptr->size()) {
						return true;
					}
					else { throw(std::out_of_range("attempt to use invalid ss_const_iterator_type - bool points_to_end_marker() const - ss_const_iterator_type - msearray")); }
				}
				else { return false; }
			}
			bool points_to_beginning() const {
				if (0 == m_index) { return true; }
				else { return false; }
			}
			/* has_next_item_or_end_marker() is just an alias for points_to_an_item(). */
			bool has_next_item_or_end_marker() const { return points_to_an_item(); } //his is
			/* has_next() is just an alias for points_to_an_item() that's familiar to java programmers. */
			bool has_next() const { return has_next_item_or_end_marker(); }
			bool has_previous() const { return (!points_to_beginning()); }
			void set_to_beginning() {
				m_index = 0;
				base_class::const_iterator::operator=(m_owner_cptr->cbegin());
			}
			void set_to_end_marker() {
				m_index = m_owner_cptr->size();
				base_class::const_iterator::operator=(m_owner_cptr->cend());
			}
			void set_to_next() {
				if (points_to_an_item()) {
					m_index += 1;
					base_class::const_iterator::operator++();
					if (m_owner_cptr->size() <= m_index) {
						if (m_owner_cptr->size() < m_index) { assert(false); reset(); }
					}
				}
				else {
					throw(std::out_of_range("attempt to use invalid const_item_pointer - void set_to_next() - ss_const_iterator_type - msearray"));
				}
			}
			void set_to_previous() {
				if (has_previous()) {
					m_index -= 1;
					base_class::const_iterator::operator--();
				}
				else {
					throw(std::out_of_range("attempt to use invalid const_item_pointer - void set_to_previous() - ss_const_iterator_type - msearray"));
				}
			}
			ss_const_iterator_type& operator ++() { (*this).set_to_next(); return (*this); }
			ss_const_iterator_type operator++(int) { ss_const_iterator_type _Tmp = *this; ++*this; return (_Tmp); }
			ss_const_iterator_type& operator --() { (*this).set_to_previous(); return (*this); }
			ss_const_iterator_type operator--(int) { ss_const_iterator_type _Tmp = *this; --*this; return (_Tmp); }
			void advance(difference_type n) {
				auto new_index = msear_int(m_index) + n;
				if ((0 > new_index) || (m_owner_cptr->size() < msear_size_t(new_index))) {
					throw(std::out_of_range("index out of range - void advance(difference_type n) - ss_const_iterator_type - msearray"));
				}
				else {
					m_index = msear_size_t(new_index);
					base_class::const_iterator::operator+=(n);
				}
			}
			void regress(difference_type n) { advance(-n); }
			ss_const_iterator_type& operator +=(difference_type n) { (*this).advance(n); return (*this); }
			ss_const_iterator_type& operator -=(difference_type n) { (*this).regress(n); return (*this); }
			ss_const_iterator_type operator+(difference_type n) const {
				ss_const_iterator_type retval; retval.m_owner_cptr = m_owner_cptr;
				retval = (*this);
				retval.advance(n);
				return retval;
			}
			ss_const_iterator_type operator-(difference_type n) const { return ((*this) + (-n)); }
			difference_type operator-(const ss_const_iterator_type &rhs) const {
				if (rhs.m_owner_cptr != (*this).m_owner_cptr) { throw(std::out_of_range("invalid argument - difference_type operator-(const ss_const_iterator_type &rhs) const - msearray::ss_const_iterator_type")); }
				auto retval = (static_cast<const typename base_class::const_iterator&>(*this) - static_cast<const typename base_class::const_iterator&>(rhs));
				assert((int)((*m_owner_cptr).size()) >= retval);
				return retval;
			}
			const_reference operator*() const {
				if (points_to_an_item()) {
					return m_owner_cptr->at(msear_as_a_size_t(m_index));
				}
				else {
					throw(std::out_of_range("attempt to use invalid const_item_pointer - const_reference operator*() const - ss_const_iterator_type - msearray"));
				}
			}
			const_reference item() const { return operator*(); }
			const_reference previous_item() const {
				if ((*this).has_previous()) {
					return m_owner_cptr->at(m_index - 1);
				}
				else {
					throw(std::out_of_range("attempt to use invalid const_item_pointer - const_reference previous_item() const - ss_const_iterator_type - msearray"));
				}
			}
			const_pointer operator->() const {
				if (points_to_an_item()) {
					const_cast<ss_const_iterator_type *>(this)->sync_const_iterator_to_index();
					//sync_const_iterator_to_index();
					return base_class::const_iterator::operator->();
				}
				else {
					throw(std::out_of_range("attempt to use invalid const_item_pointer - pointer operator->() const - ss_const_iterator_type - msearray"));
				}
			}
			const_reference operator[](difference_type _Off) const { return (*(*this + _Off)); }
			/*
			ss_const_iterator_type& operator=(const typename base_class::const_iterator& _Right_cref)
			{
			msear_int d = std::distance<typename base_class::iterator>(m_owner_cptr->cbegin(), _Right_cref);
			if ((0 <= d) && (m_owner_cptr->size() >= d)) {
			if (m_owner_cptr->size() == d) {
			assert(m_owner_cptr->cend() == _Right_cref);
			}
			m_index = msear_size_t(d);
			base_class::const_iterator::operator=(_Right_cref);
			}
			else {
			throw(std::out_of_range("doesn't seem to be a valid assignment value - ss_const_iterator_type& operator=(const typename base_class::const_iterator& _Right_cref) - ss_const_iterator_type - msearray"));
			}
			return (*this);
			}
			*/
			ss_const_iterator_type& operator=(const ss_const_iterator_type& _Right_cref) {
				((*this).m_owner_cptr) = _Right_cref.m_owner_cptr;
				(*this).m_index = _Right_cref.m_index;
				base_class::const_iterator::operator=(_Right_cref);
				return (*this);
			}
			bool operator==(const ss_const_iterator_type& _Right_cref) const { return ((_Right_cref.m_index == m_index) && (_Right_cref.m_owner_cptr == m_owner_cptr)); }
			bool operator!=(const ss_const_iterator_type& _Right_cref) const { return (!(_Right_cref == (*this))); }
			bool operator<(const ss_const_iterator_type& _Right) const {
				if (this->m_owner_cptr != _Right.m_owner_cptr) { throw(std::out_of_range("invalid argument - ss_const_iterator_type& operator<(const ss_const_iterator_type& _Right) - ss_const_iterator_type - msearray")); }
				return (m_index < _Right.m_index);
			}
			bool operator<=(const ss_const_iterator_type& _Right) const { return (((*this) < _Right) || (_Right == (*this))); }
			bool operator>(const ss_const_iterator_type& _Right) const { return (!((*this) <= _Right)); }
			bool operator>=(const ss_const_iterator_type& _Right) const { return (!((*this) < _Right)); }
			void set_to_const_item_pointer(const ss_const_iterator_type& _Right_cref) {
				(*this) = _Right_cref;
			}
			void invalidate_inclusive_range(msear_size_t index_of_first, msear_size_t index_of_last) {
				if ((index_of_first <= (*this).m_index) && (index_of_last >= (*this).m_index)) {
					(*this).reset();
				}
			}
			void shift_inclusive_range(msear_size_t index_of_first, msear_size_t index_of_last, msear_int shift) {
				if ((index_of_first <= (*this).m_index) && (index_of_last >= (*this).m_index)) {
					auto new_index = (*this).m_index + shift;
					if ((0 > new_index) || (m_owner_cptr->size() < new_index)) {
						throw(std::out_of_range("void shift_inclusive_range() - ss_const_iterator_type - msearray"));
					}
					else {
						(*this).m_index = msear_size_t(new_index);
						(*this).sync_const_iterator_to_index();
					}
				}
			}
			msear_size_t position() const {
				return m_index;
			}
		private:
			void sync_const_iterator_to_index() {
				assert(m_owner_cptr->size() >= (*this).m_index);
				base_class::const_iterator::operator=(m_owner_cptr->cbegin());
				base_class::const_iterator::operator+=(msear_as_a_size_t(m_index));
			}
			msear_size_t m_index = 0;
			msear_pointer<const _Myt> m_owner_cptr = nullptr;
			friend class /*_Myt*/msearray<_Ty, _Size>;
		};
		/* Note that, at the moment, ss_iterator_type inherits publicly from base_class::iterator. This is not intended to be a permanent
		characteristc of ss_iterator_type and any reference to, or interpretation of, an ss_iterator_type as an base_class::iterator is (and has
		always been) depricated. ss_iterator_type endeavors to support (and continue to support) the subset of the base_class::iterator
		interface that is compatible with the security/safety goals of ss_iterator_type.
		In particular, keep in mind that base_class::iterator does not have a virtual destructor, so deallocating an ss_iterator_type as an
		base_class::iterator would result in memory leaks. */
		class ss_iterator_type : public base_class::iterator {
		public:
			typedef typename base_class::iterator::iterator_category iterator_category;
			typedef typename base_class::iterator::value_type value_type;
			typedef typename base_class::iterator::difference_type difference_type;
			typedef difference_type distance_type;	// retained
			typedef typename base_class::iterator::pointer pointer;
			typedef typename base_class::iterator::reference reference;

			ss_iterator_type() {}
			void reset() { set_to_end_marker(); }
			bool points_to_an_item() const {
				if ((1 <= m_owner_ptr->size()) && (m_index < m_owner_ptr->size())) { return true; }
				else {
					if (m_index == m_owner_ptr->size()) { return false; }
					else { throw(std::out_of_range("attempt to use invalid ss_iterator_type - bool points_to_an_item() const - ss_iterator_type - msearray")); }
				}
			}
			bool points_to_end_marker() const {
				if (false == points_to_an_item()) {
					if (m_index == m_owner_ptr->size()) {
						return true;
					}
					else { throw(std::out_of_range("attempt to use invalid ss_iterator_type - bool points_to_end_marker() const - ss_iterator_type - msearray")); }
				}
				else { return false; }
			}
			bool points_to_beginning() const {
				if (0 == m_index) { return true; }
				else { return false; }
			}
			/* has_next_item_or_end_marker() is just an alias for points_to_an_item(). */
			bool has_next_item_or_end_marker() const { return points_to_an_item(); }
			/* has_next() is just an alias for points_to_an_item() that's familiar to java programmers. */
			bool has_next() const { return has_next_item_or_end_marker(); }
			bool has_previous() const { return (!points_to_beginning()); }
			void set_to_beginning() {
				m_index = 0;
				base_class::iterator::operator=(m_owner_ptr->begin());
			}
			void set_to_end_marker() {
				m_index = m_owner_ptr->size();
				base_class::iterator::operator=(m_owner_ptr->end());
			}
			void set_to_next() {
				if (points_to_an_item()) {
					m_index += 1;
					base_class::iterator::operator++();
					if (m_owner_ptr->size() <= m_index) {
						if (m_owner_ptr->size() < m_index) { assert(false); reset(); }
					}
				}
				else {
					throw(std::out_of_range("attempt to use invalid item_pointer - void set_to_next() - ss_const_iterator_type - msearray"));
				}
			}
			void set_to_previous() {
				if (has_previous()) {
					m_index -= 1;
					base_class::iterator::operator--();
				}
				else {
					throw(std::out_of_range("attempt to use invalid item_pointer - void set_to_previous() - ss_iterator_type - msearray"));
				}
			}
			ss_iterator_type& operator ++() { (*this).set_to_next(); return (*this); }
			ss_iterator_type operator++(int) { ss_iterator_type _Tmp = *this; ++*this; return (_Tmp); }
			ss_iterator_type& operator --() { (*this).set_to_previous(); return (*this); }
			ss_iterator_type operator--(int) { ss_iterator_type _Tmp = *this; --*this; return (_Tmp); }
			void advance(difference_type n) {
				auto new_index = msear_int(m_index) + n;
				if ((0 > new_index) || (m_owner_ptr->size() < msear_size_t(new_index))) {
					throw(std::out_of_range("index out of range - void advance(difference_type n) - ss_iterator_type - msearray"));
				}
				else {
					m_index = msear_size_t(new_index);
					base_class::iterator::operator+=(n);
				}
			}
			void regress(difference_type n) { advance(-n); }
			ss_iterator_type& operator +=(difference_type n) { (*this).advance(n); return (*this); }
			ss_iterator_type& operator -=(difference_type n) { (*this).regress(n); return (*this); }
			ss_iterator_type operator+(difference_type n) const {
				ss_iterator_type retval; retval.m_owner_ptr = m_owner_ptr;
				retval = (*this);
				retval.advance(n);
				return retval;
			}
			ss_iterator_type operator-(difference_type n) const { return ((*this) + (-n)); }
			difference_type operator-(const ss_iterator_type& rhs) const {
				if (rhs.m_owner_ptr != (*this).m_owner_ptr) { throw(std::out_of_range("invalid argument - difference_type operator-(const ss_iterator_type& rhs) const - msearray::ss_iterator_type")); }
				auto retval = (static_cast<const typename base_class::iterator&>(*this) - static_cast<const typename base_class::iterator&>(rhs));
				assert((int)((*m_owner_ptr).size()) >= retval);
				return retval;
			}
			reference operator*() {
				if (points_to_an_item()) {
					return m_owner_ptr->at(msear_as_a_size_t(m_index));
				}
				else {
					throw(std::out_of_range("attempt to use invalid item_pointer - reference operator*() - ss_iterator_type - msearray"));
				}
			}
			reference item() { return operator*(); }
			reference previous_item() {
				if ((*this).has_previous()) {
					return m_owner_ptr->at(m_index - 1);
				}
				else {
					throw(std::out_of_range("attempt to use invalid item_pointer - reference previous_item() - ss_const_iterator_type - msearray"));
				}
			}
			pointer operator->() {
				if (points_to_an_item()) {
					sync_iterator_to_index();
					return base_class::iterator::operator->();
				}
				else {
					throw(std::out_of_range("attempt to use invalid item_pointer - pointer operator->() - ss_iterator_type - msearray"));
				}
			}
			reference operator[](difference_type _Off) { return (*(*this + _Off)); }
			/*
			ss_iterator_type& operator=(const typename base_class::iterator& _Right_cref)
			{
			msear_int d = std::distance<typename base_class::iterator>(m_owner_ptr->begin(), _Right_cref);
			if ((0 <= d) && (m_owner_ptr->size() >= d)) {
			if (m_owner_ptr->size() == d) {
			assert(m_owner_ptr->end() == _Right_cref);
			}
			m_index = msear_size_t(d);
			base_class::iterator::operator=(_Right_cref);
			}
			else {
			throw(std::out_of_range("doesn't seem to be a valid assignment value - ss_iterator_type& operator=(const typename base_class::iterator& _Right_cref) - ss_const_iterator_type - msearray"));
			}
			return (*this);
			}
			*/
			ss_iterator_type& operator=(const ss_iterator_type& _Right_cref) {
				((*this).m_owner_ptr) = _Right_cref.m_owner_ptr;
				(*this).m_index = _Right_cref.m_index;
				base_class::iterator::operator=(_Right_cref);
				return (*this);
			}
			bool operator==(const ss_iterator_type& _Right_cref) const { return ((_Right_cref.m_index == m_index) && (_Right_cref.m_owner_ptr == m_owner_ptr)); }
			bool operator!=(const ss_iterator_type& _Right_cref) const { return (!(_Right_cref == (*this))); }
			bool operator<(const ss_iterator_type& _Right) const {
				if (this->m_owner_ptr != _Right.m_owner_ptr) { throw(std::out_of_range("invalid argument - ss_iterator_type& operator<(const ss_iterator_type& _Right) - ss_iterator_type - msearray")); }
				return (m_index < _Right.m_index);
			}
			bool operator<=(const ss_iterator_type& _Right) const { return (((*this) < _Right) || (_Right == (*this))); }
			bool operator>(const ss_iterator_type& _Right) const { return (!((*this) <= _Right)); }
			bool operator>=(const ss_iterator_type& _Right) const { return (!((*this) < _Right)); }
			void set_to_item_pointer(const ss_iterator_type& _Right_cref) {
				(*this) = _Right_cref;
			}
			void invalidate_inclusive_range(msear_size_t index_of_first, msear_size_t index_of_last) {
				if ((index_of_first <= (*this).m_index) && (index_of_last >= (*this).m_index)) {
					(*this).reset();
				}
			}
			void shift_inclusive_range(msear_size_t index_of_first, msear_size_t index_of_last, msear_int shift) {
				if ((index_of_first <= (*this).m_index) && (index_of_last >= (*this).m_index)) {
					auto new_index = (*this).m_index + shift;
					if ((0 > new_index) || (m_owner_ptr->size() < new_index)) {
						throw(std::out_of_range("void shift_inclusive_range() - ss_iterator_type - msearray"));
					}
					else {
						(*this).m_index = msear_size_t(new_index);
						(*this).sync_iterator_to_index();
					}
				}
			}
			msear_size_t position() const {
				return m_index;
			}
			operator ss_const_iterator_type() const {
				ss_const_iterator_type retval;
				if (m_owner_ptr == nullptr) {
					retval = m_owner_ptr->ss_cbegin();
					retval.advance(msear_int(m_index));
				}
				return retval;
			}
		private:
			void sync_iterator_to_index() {
				assert(m_owner_ptr->size() >= (*this).m_index);
				base_class::iterator::operator=(m_owner_ptr->begin());
				base_class::iterator::operator+=(msear_as_a_size_t(m_index));
			}
			msear_size_t m_index = 0;
			msear_pointer<_Myt> m_owner_ptr = nullptr;
			friend class /*_Myt*/msearray<_Ty, _Size>;
		};
		typedef std::reverse_iterator<ss_iterator_type> ss_reverse_iterator_type;
		typedef std::reverse_iterator<ss_const_iterator_type> ss_const_reverse_iterator_type;

		ss_iterator_type ss_begin()
		{	// return base_class::iterator for beginning of mutable sequence
			ss_iterator_type retval; retval.m_owner_ptr = this;
			retval.set_to_beginning();
			return retval;
		}

		ss_const_iterator_type ss_begin() const
		{	// return base_class::iterator for beginning of nonmutable sequence
			ss_const_iterator_type retval; retval.m_owner_cptr = this;
			retval.set_to_beginning();
			return retval;
		}

		ss_iterator_type ss_end()
		{	// return base_class::iterator for end of mutable sequence
			ss_iterator_type retval; retval.m_owner_ptr = this;
			retval.set_to_end_marker();
			return retval;
		}

		ss_const_iterator_type ss_end() const
		{	// return base_class::iterator for end of nonmutable sequence
			ss_const_iterator_type retval; retval.m_owner_cptr = this;
			retval.set_to_set_to_end_marker();
			return retval;
		}

		ss_const_iterator_type ss_cbegin() const
		{	// return base_class::iterator for beginning of nonmutable sequence
			ss_const_iterator_type retval; retval.m_owner_cptr = this;
			retval.set_to_beginning();
			return retval;
		}

		ss_const_iterator_type ss_cend() const
		{	// return base_class::iterator for end of nonmutable sequence
			ss_const_iterator_type retval; retval.m_owner_cptr = this;
			retval.set_to_set_to_end_marker();
			return retval;
		}

		ss_const_reverse_iterator_type ss_crbegin() const
		{	// return base_class::iterator for beginning of reversed nonmutable sequence
			return (ss_rbegin());
		}

		ss_const_reverse_iterator_type ss_crend() const
		{	// return base_class::iterator for end of reversed nonmutable sequence
			return (ss_rend());
		}

		ss_reverse_iterator_type ss_rbegin()
		{	// return base_class::iterator for beginning of reversed mutable sequence
			return (reverse_iterator(ss_end()));
		}

		ss_const_reverse_iterator_type ss_rbegin() const
		{	// return base_class::iterator for beginning of reversed nonmutable sequence
			return (const_reverse_iterator(ss_end()));
		}

		ss_reverse_iterator_type ss_rend()
		{	// return base_class::iterator for end of reversed mutable sequence
			return (reverse_iterator(ss_begin()));
		}

		ss_const_reverse_iterator_type ss_rend() const
		{	// return base_class::iterator for end of reversed nonmutable sequence
			return (const_reverse_iterator(ss_begin()));
		}
	};

	template<class _Ty, size_t _Size>
	struct std::tuple_size<mse::msearray<_Ty, _Size> >
		: integral_constant<size_t, _Size>
	{	// struct to determine number of elements in array
	};

	template<size_t _Idx, class _Ty, size_t _Size>
	struct std::tuple_element<_Idx, mse::msearray<_Ty, _Size> >
	{	// struct to determine type of element _Idx in array
		static_assert(_Idx < _Size, "array index out of bounds");

		typedef _Ty type;
	};

	class msearray_test {
	public:
		void test1() {
#ifdef MSE_SELF_TESTS
			// construction uses aggregate initialization
			mse::msearray<int, 3> a1{ { 1, 2, 3 } }; // double-braces required in C++11 (not in C++14)
			mse::msearray<int, 3> a2 = { 11, 12, 13 };  // never required after =
			mse::msearray<std::string, 2> a3 = { std::string("a"), "b" };

			// container operations are supported
			std::sort(a1.begin(), a1.end());
			std::reverse_copy(a2.begin(), a2.end(),
				std::ostream_iterator<int>(std::cout, " "));

			std::cout << '\n';

			// ranged for loop is supported
			for (const auto& s : a3)
				std::cout << s << ' ';

			a1.swap(a2);
			assert(13 == a1[2]);
			assert(3 == a2[2]);

			std::swap(a1, a2);
			assert(3 == a1[2]);
			assert(13 == a2[2]);

			std::get<0>(a1) = 21;
			std::get<1>(a1) = 22;
			std::get<2>(a1) = 23;

			auto l_tuple_size = std::tuple_size<mse::msearray<int, 3>>::value;
			std::tuple_element<1, mse::msearray<int, 3>>::type b1 = 5;

#endif // MSE_SELF_TESTS
		}
	};
}
#endif /*ndef MSEMSEARRAY_H*/
