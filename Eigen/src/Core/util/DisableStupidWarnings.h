#ifndef EIGEN_WARNINGS_DISABLED
#define EIGEN_WARNINGS_DISABLED

#ifdef _MSC_VER
  // 4100 - unreferenced formal parameter (occurred e.g. in aligned_allocator::destroy(pointer p))
  // 4101 - unreferenced local variable
  // 4127 - conditional expression is constant
  // 4181 - qualifier applied to reference type ignored
  // 4211 - nonstandard extension used : redefined extern to static
  // 4244 - 'argument' : conversion from 'type1' to 'type2', possible loss of data
  // 4273 - QtAlignedMalloc, inconsistent DLL linkage
  // 4324 - structure was padded due to declspec(align())
  // 4512 - assignment operator could not be generated
  // 4522 - 'class' : multiple assignment operators specified
  // 4700 - uninitialized local variable 'xyz' used
  // 4717 - 'function' : recursive on all control paths, function will cause runtime stack overflow
  #ifndef EIGEN_PERMANENTLY_DISABLE_STUPID_WARNINGS
    #pragma warning( push )
  #endif
  #pragma warning( disable : 4100 4101 4127 4181 4211 4244 4273 4324 4512 4522 4700 4717 )
#elif defined __INTEL_COMPILER
  // 2196 - routine is both "inline" and "noinline" ("noinline" assumed)
  //        ICC 12 generates this warning even without any inline keyword, when defining class methods 'inline' i.e. inside of class body
  // 2536 - type qualifiers are meaningless here
  //        ICC 12 generates this warning when a function return type is const qualified, even if that type is a template-parameter-dependent
  //        typedef that may be a reference type.
  #ifndef EIGEN_PERMANENTLY_DISABLE_STUPID_WARNINGS
    #pragma warning push
  #endif
  #pragma warning disable 2196 2536
#endif

#endif // not EIGEN_WARNINGS_DISABLED
