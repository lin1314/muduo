/*************************************************************************
  > File Name: Singleton.h
  > Author: ljy
  > Mail: 2818880298@qq.com
  > Created Time: Sat 20 Jan 2018 02:50:13 PM CST
 ************************************************************************/

#ifndef _SINGLETON_H
#define _SINGLETON_H
template <typename T> class Singleton
{
private:
  Singleton(const Singleton<T>&);
  Singleton& operator=(const Singleton<T>&);

protected:
  static T* msSingleton;

public:
  Singleton( void ) {
    assert( !msSingleton );
#if defined( _MSC_VER ) && _MSC_VER < 1200
    int offset = (int)(T*)1 - (int)(Singleton <T>*)(T*)1;
    msSingleton = (T*)((int)this + offset);
#else
    msSingleton = static_cast< T* >( this );
#endif
  }

  ~Singleton( void ) {
    assert( msSingleton );
    msSingleton = 0;
  }
  static T& getSingleton( void ) {
    assert( msSingleton ); return ( *msSingleton );
  }

  static T* getSingletonPtr( void ) {
    return msSingleton;
  }
};

template<typename T>
T* Singleton<T>::msSingleton = 0;

#endif

