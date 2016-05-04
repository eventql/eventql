## TODO

- Promise<T>::failure():
  - support for storing native/RuntimeError exception
  - to be raised on get().
- stx-unittest
  - add `ASSERT_`-macros
  - add `ASSERT_NEAR()` and `EXPECT_NEAR()`
  - test suite name should be able to match the name of an existing symbol
  - `EXPECT_THROW()` differs from `EXPECT_EXCEPTION()` (no easy porting possible)
- kStatus should contain new things from cortex' Status
