// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>
#include <cortex-base/thread/Queue.h>

using namespace cortex;

TEST(Queue, poll) {
  thread::Queue<int> queue;
  Option<int> o = queue.poll();
  ASSERT_TRUE(o.isNone());

  queue.insert(42);
  o = queue.poll();
  ASSERT_TRUE(o.isSome());
  ASSERT_EQ(42, o.get());
}

TEST(Queue, pop) {
  thread::Queue<int> queue;
  queue.insert(42);

  // TODO: make sure this was kinda instant
  Option<int> o = queue.pop();
  ASSERT_TRUE(o.isSome());
  ASSERT_EQ(42, o.get());

  // TODO: queue.pop() and have a bg thread inserting something 1s later.
  // We expect pop to complete after 1s.
}
