#pragma once


template<class... Tn>
struct List;

template<class, class...>
struct Find;

template<class T, class... Tn>
struct Find<List<T, Tn...>, T> {
	static int const value = 0;
};

template<class T1, class... Tn, class T>
struct Find<List<T1, Tn...>, T> {
	static int const value = 1 + Find<List<Tn...>, T>::value;
};

template<class T>
struct FalseWrapper {
	FalseWrapper( T const& v ): value( v ) {
	}

	operator bool() const {
		return false;
	}

	T value;
};

template<class Base, class... Tn>
struct Variant {
	Variant() {}

	template<class T>
	Variant( T* p ):
		_ptr( p ),
		_tag( Tag<T>::value ) {}

	template<class T>
	struct Tag {
		static int const value = Find<List<Tn...>, T>::value;
	};

	// private:
		Base* _ptr;
		int _tag;
};

#define VMATCH( val ) \
	if( FalseWrapper<decltype((val)._ptr)> _variant_switch_value_ = (val)._ptr ) {} else \
	switch( (val)._tag ) { \
		typedef decltype(val) _variant_switch_type_;

#define VCASE( type, var ) \
			break; \
		} \
		case _variant_switch_type_::Tag<type>::value: { \
			type* var = static_cast<type*>( _variant_switch_value_.value );

#define VDEFAULT \
			break; \
		} \
		default:
