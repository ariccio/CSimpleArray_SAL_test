#include "stdafx.h"



#pragma push_macro("malloc")
#undef malloc
#pragma push_macro("calloc")
#undef calloc
#pragma push_macro("realloc")
#undef realloc
#pragma push_macro("_recalloc")
#undef _recalloc
#pragma push_macro("free")
#undef free

#pragma warning(push)
#pragma warning(disable: 4800) // forcing 'int' value to bool


#pragma pack(push,_ATL_PACKING)
namespace myATL
{

#pragma push_macro("new")
#undef new

/////////////////////////////////////////////////////////////////////////////
// Collection helpers - CSimpleArray & CSimpleMap

ATLPREFAST_SUPPRESS(6319)
// template class helpers with functions for comparing elements
// override if using complex types without operator==
template <class T>
class CSimpleArrayEqualHelper
{
public:
	static bool IsEqual(
		_In_ const T& t1,
		_In_ const T& t2)
	{
		return (t1 == t2);
	}
};
ATLPREFAST_UNSUPPRESS()

template <class T>
class CSimpleArrayEqualHelperFalse
{
public:
	static bool IsEqual(
		_In_ const T&,
		_In_ const T&)
	{
		ATLASSERT(false);
		return false;
	}
};


template <class T, class TEqual = CSimpleArrayEqualHelper< T > >
class CSimpleArray
{
public:
// Construction/destruction
	//_At_( m_aT, _Post_ptr_invalid_ )
	_At_( m_aT, _Post_equal_to_( NULL ) )
	_At_( m_aT, _Post_count_c_( 0 ) )
	_At_( m_nSize, _Post_satisfies_( m_nSize == 0 ) )
	CSimpleArray() :
		m_aT(NULL), m_nSize(0), m_nAllocSize(0)
	{
	}

	_At_( m_aT, _Post_ptr_invalid_ )
	~CSimpleArray();

	//_When_( src.m_nSize > 0, _At_( m_aT, _Post_readable_size_( src.m_nSize ) ) )
	CSimpleArray(_In_ const CSimpleArray< T, TEqual >& src) :
		m_aT(NULL), m_nSize(0), m_nAllocSize(0)
	{
        if (src.GetSize())
        {
			m_aT = (T*)calloc(src.GetSize(), sizeof(T));
			if (m_aT != NULL)
			{
				m_nAllocSize = src.GetSize();
				for (int i=0; i<src.GetSize(); i++)
					Add(src[i]);
			}
		}
	}

	_When_( src.m_nSize > 0, _At_( m_aT, _Post_readable_size_( src.m_nSize ) ) )
	CSimpleArray< T, TEqual >& operator=(_In_ const CSimpleArray< T, TEqual >& src)
	{
		if (GetSize() != src.GetSize())
		{
			RemoveAll();
			m_aT = (T*)calloc(src.GetSize(), sizeof(T));
			if (m_aT != NULL)
				m_nAllocSize = src.GetSize();
		}
		else
		{
			for (int i = GetSize(); i > 0; i--)
				RemoveAt(i - 1);
		}
		for (int i=0; i<src.GetSize(); i++)
			Add(src[i]);
		return *this;
	}

// Operations
	_At_( m_aT, _Post_readable_size_( m_nSize ) )
	_At_( m_aT, _Post_writable_size_( m_nSize ) )
	int GetSize() const
	{
		return m_nSize;
	}

	_Success_( return == TRUE )
	_At_( m_nSize, _Post_satisfies_( m_nSize == ( _Old_( m_nSize ) + 1 ) ) ) 
	_At_( m_aT, _Post_count_x_( ( _Old_( m_nSize ) + 1 ) ) ) 
	//_At_( m_aT, _When_( m_aT == NULL, _Post_satisfies_( m_aT != NULL ) ) )
	BOOL Add(_In_ const T& t)
	{
		if(m_nSize == m_nAllocSize)
		{
			// Make sure newElement is not a reference to an element in the array.
			// Or else, it will be invalidated by the reallocation.
			ATLENSURE(	(&t < m_aT) ||
						(&t >= (m_aT + m_nAllocSize) ) );

			T* aT;
			int nNewAllocSize = (m_nAllocSize == 0) ? 1 : (m_nSize * 2);

			if (nNewAllocSize<0||nNewAllocSize>INT_MAX/sizeof(T))
			{
				return FALSE;
			}

			aT = (T*)_recalloc(m_aT, nNewAllocSize, sizeof(T));
			if(aT == NULL)
				return FALSE;
			m_nAllocSize = nNewAllocSize;
			m_aT = aT;
		}
		InternalSetAtIndex(m_nSize, t);
		m_nSize++;
		return TRUE;
	}

	_Success_( return == TRUE )
	_Post_satisfies_( m_nSize == ( _Old_( m_nSize ) - 1 ) )
	BOOL Remove(_In_ const T& t)
	{
		int nIndex = Find(t);
		if(nIndex == -1)
			return FALSE;
		return RemoveAt(nIndex);
	}
	
	_Success_( return == TRUE )
	_Pre_satisfies_( m_nSize > 0 )
	_At_( m_aT, _Post_count_x_( ( _Old_( m_nSize ) - 1 ) ) )
	BOOL RemoveAt(_In_ _In_range_( 0, m_nSize ) int nIndex)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		if (nIndex < 0 || nIndex >= m_nSize)
			return FALSE;
		m_aT[nIndex].~T();
		if(nIndex != (m_nSize - 1))
			Checked::memmove_s((void*)(m_aT + nIndex), (m_nSize - nIndex) * sizeof(T), (void*)(m_aT + nIndex + 1), (m_nSize - (nIndex + 1)) * sizeof(T));
		m_nSize--;
		return TRUE;
	}

	//_At_( this->m_aT, _Post_count_c_( 0 ) )
	//_At_( this->m_nSize, _Post_equal_to_( 0 ) )
	_At_( this->m_aT, _Post_invalid_ )
	void RemoveAll()
	{
		if(m_aT != NULL)
		{
			for(int i = 0; i < m_nSize; i++)
				m_aT[i].~T();
			free(m_aT);
			m_aT = NULL;
		}
		m_nSize = 0;
		m_nAllocSize = 0;
    }

	_At_( nIndex, _In_range_( 0, this->m_nSize ) )
	_At_( this->m_aT, _Pre_count_( this->m_nSize ) )
	const T& operator[] (_In_ int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		if(nIndex < 0 || nIndex >= m_nSize)
		{
			_AtlRaiseException((DWORD)EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
		}
		return m_aT[nIndex];
	}

	_At_( nIndex, _In_range_( 0, m_nSize ) )
	_At_( m_aT, _Pre_readable_size_( m_nSize ) )
	_At_( m_aT, _Pre_writable_size_( m_nSize ) )
	T& operator[] (_In_ int nIndex)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		if(nIndex < 0 || nIndex >= m_nSize)
		{
			_AtlRaiseException((DWORD)EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
		}
		return m_aT[nIndex];
	}

	//_Ret_maybenull_
	_When_( this->m_aT == NULL, _Ret_null_ )
	_When_( this->m_aT != NULL, _Ret_notnull_ )
	_When_( this->m_aT != NULL, _At_( return, _Post_count_( this->m_nSize ) ) )
	_When_( this->m_aT != NULL, _At_( return, _Post_cap_( this->m_nAllocSize ) ) )
	_When_( return != NULL, _At_( return, _Post_count_( this->m_nSize ) ) )
	_When_( return != NULL, _At_( return, _Post_cap_( this->m_nAllocSize ) ) )
	_Ret_count_x_( m_nSize )
	T* GetData() const
	{
		return m_aT;
	}

	_Success_( return != -1 )
	_Pre_satisfies_( this->m_aT != NULL )
	_Ret_range_( 0, this->m_nSize )
	int Find(_In_ const T& t) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
			if(TEqual::IsEqual(m_aT[i], t))
				return i;
		}
		return -1;  // not found
	}

	_Success_( return == TRUE )
	_At_( nIndex, _In_range_( 0, this->m_nSize ) )
	_At_( m_aT, _Pre_cap_( this->m_nSize ) )
	_Check_return_
	BOOL SetAtIndex(
		_In_ int nIndex,
		_In_ const T& t)
	{
		if (nIndex < 0 || nIndex >= m_nSize)
			return FALSE;
		InternalSetAtIndex(nIndex, t);
		return TRUE;
	}

// Implementation
	class Wrapper
	{
	public:
		Wrapper(_In_ const T& _t) : t(_t)
		{
		}
		template <class _Ty>
		void * __cdecl operator new(
			_In_ size_t,
			_In_ _Ty* p)
		{
			return p;
		}
		template <class _Ty>
		void __cdecl operator delete(
			_In_ void* /* pv */,
			_In_ _Ty* /* p */)
		{
		}
		T t;
	};

// Implementation
	_At_( nIndex, _In_range_( 0, m_nAllocSize ) )
	void InternalSetAtIndex(
		_In_ int nIndex,
		_In_ const T& t)
	{
		new(m_aT + nIndex) Wrapper(t);
	}

	typedef T _ArrayElementType;
	_Field_size_part_( m_nSize, m_nAllocSize ) T* m_aT;
	_Field_range_( 0, m_nAllocSize )int m_nSize;
	int m_nAllocSize;
};

#define CSimpleValArray CSimpleArray

template <class T, class TEqual> inline CSimpleArray<T, TEqual>::~CSimpleArray()
{
	RemoveAll();
}


#pragma pop_macro("new")

};  // namespace ATL
#pragma pack(pop)

#pragma warning(pop)

#pragma pop_macro("free")
#pragma pop_macro("realloc")
#pragma pop_macro("_recalloc")
#pragma pop_macro("malloc")
#pragma pop_macro("calloc")



CWinApp theApp;

_At_( in, _In_range_( 10, 100 ) )
void test_analyze_ten_or_onehundred( int in ) {
	printf( "%i\r\n", in );
	ASSERT( in >= 10 ); ASSERT( in <= 100 );
	}

_At_( in, _In_range_( 1, 2 ) )
void test_analyze_one_or_two( int in ) {
	printf( "%i\r\n", in );
	ASSERT( in >= 1 ); ASSERT( in <= 2 );
	}

_At_( in, _In_range_( 2, 3 ) )
void test_analyze_two_or_three( int in ) {
	printf( "%i\r\n", in );
	ASSERT( in >= 2 ); ASSERT( in <= 3 );
	}


_At_( in, _In_range_( 0, 0 ) )
void test_analyze_zero( int in ) {
	printf( "%i\r\n", in );
	ASSERT( in >= 0 );
	}

int wmain( int argc, _In_reads_( argc ) _Readable_elements_( argc ) WCHAR* argv[ ], WCHAR* envp[ ] ) {
	//printf( "start!\r\n" );
	HMODULE hModule = ::GetModuleHandleW( NULL );
	int nRetCode = 0;

	if ( hModule == NULL ) {
		fwprintf( stderr, L"Couldn't get module handle!\r\n" );
		return ERROR_MOD_NOT_FOUND;
		}

	if ( !AfxWinInit( hModule, NULL, ::GetCommandLine( ), 0 ) ) {
		fwprintf( stderr, L"Fatal Error: MFC initialization failed!\r\n" );
		return ERROR_APP_INIT_FAILURE;
		}

	//built-in CSimpleArray
	ATL::CSimpleArray<int> b;

	b.Add( 5 );

	b.RemoveAll( );
	//With SAL, /analyze will NOT catch this
	const auto c = b[ 2 ];





	//modified CSimpleArray
	myATL::CSimpleArray<int> d;

	//this should not fail!
	test_analyze_zero( d.m_nSize );


	//these should fail!
	test_analyze_ten_or_onehundred( d.m_nSize );
	test_analyze_one_or_two( d.m_nSize );
	test_analyze_two_or_three( d.m_nSize );



	//this should fail!
	const auto f = d[ 20 ];

	if ( !d.Add( 5 ) ) {
		fwprintf( stderr, L"d.Add( 5 ) failed!\r\n" );
		return ERROR_FUNCTION_FAILED;
		}

	//this should not fail!
	const auto t = d[ 0 ];

	//these should fail!
	const auto u = d[ 1 ];
	test_analyze_zero( d.m_nSize );

	d.RemoveAll( );
	

	//this should fail!
	//With SAL, /analyze will catch this
	const auto e = d[ 1 ];
	
	//this should fail!
	test_analyze_one_or_two( d.m_nSize );



	if ( !d.Add( 5 ) ) {
		fwprintf( stderr, L"d.Add( 5 ) failed!\r\n" );
		return ERROR_FUNCTION_FAILED;
		}
	//there is now one item in d

	//this should not fail!
	const auto p = d[ 0 ];
	
	//these should fail!
	const auto g = d[ 1 ];
	test_analyze_two_or_three( d.m_nSize );

	//this should not fail!
	test_analyze_one_or_two( d.m_nSize );


	if ( !d.Add( 5 ) ) {
		fwprintf( stderr, L"d.Add( 5 ) failed!\r\n" );
		return ERROR_FUNCTION_FAILED;
		}
	if ( !d.Add( 5 ) ) {
		fwprintf( stderr, L"d.Add( 5 ) failed!\r\n" );
		return ERROR_FUNCTION_FAILED;
		}
	//there are now three items in d


	//these should not fail
	const auto k = d[ 1 ];
	const auto l = d[ 2 ];

	//these should fail!
	const auto m = d[ 4 ];
	const auto q = d[ d.GetSize( ) ];
	test_analyze_ten_or_onehundred( d.m_nSize );
	test_analyze_one_or_two( d.m_nSize );

	//this should not fail!
	test_analyze_two_or_three( d.m_nSize );

	//confirms that `/analyze`'s internal model thinks d is an array of 0 elements (should fail!)
	test_analyze_zero( d.m_nSize );

	//For some reason, `/analyze`'s internal model thinks h is an array of 0 bytes.
	//It seems to think that `this->m_nSize` == 0 bytes at the call.
	//Actual text:
	/*
	C6385	Read overrun	Reading invalid data from 'h':  the readable size is 'this->m_nSize*4' bytes, but '12' bytes may be read.
	
	368 'h' is a 0 byte array
	369 Invalid read from 'h', (outside its readable range)
	
	*/

	const int z_ = 5;

	if ( !d.SetAtIndex( 0, z_ ) ) {
		fwprintf( stderr, L"d.SetAtIndex( 0, z_ )" );
		return ERROR_FUNCTION_FAILED;
		}

	if ( !d.SetAtIndex( 1, z_ ) ) {
		fwprintf( stderr, L"d.SetAtIndex( 0, z_ )" );
		return ERROR_FUNCTION_FAILED;
		}

	if ( !d.SetAtIndex( 2, z_ ) ) {
		fwprintf( stderr, L"d.SetAtIndex( 0, z_ )" );
		return ERROR_FUNCTION_FAILED;
		}


	const auto h = d.GetData( );

	//this should not fail!
	const auto i = h[ 2 ];

	//this should fail!
	const auto j = h[ 3 ];


	const auto other_array = d;

	//this should not fail!
	const auto r = other_array[ 2 ];

	//this should fail!
	const auto s = other_array[ 3 ];



	return nRetCode;
	}