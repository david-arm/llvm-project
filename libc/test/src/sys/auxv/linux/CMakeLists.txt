add_custom_target(libc_sys_auxv_unittests)
add_libc_unittest(
  getauxval_test
  SUITE
    libc_sys_auxv_unittests
  SRCS
    getauxval_test.cpp
  DEPENDS
    libc.include.sys_auxv
    libc.src.errno.errno
    libc.src.sys.auxv.getauxval
    libc.test.UnitTest.ErrnoCheckingTest
    libc.test.UnitTest.ErrnoSetterMatcher
    libc.src.string.strstr
)
