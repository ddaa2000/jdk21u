/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_GC_G1_G1OOPQUEUE_HPP
#define SHARE_GC_G1_G1OOPQUEUE_HPP

// #include "oops/oop.hpp"
#include "oops/oopsHierarchy.hpp"
#include "runtime/threadSMR.hpp"
#include "utilities/sizes.hpp"

class G1ThreadLocalData;
class ReferenceHashMap;

class G1OopQueue : CHeapObj<mtGC> {
private:
  oopDesc** _buffer;
  size_t _index;
  static const size_t _element_size = sizeof(oopDesc*);
  size_t _total_count, _young_count, _identical_count;

public:
  G1OopQueue();
  ~G1OopQueue();

  void flush(ReferenceHashMap& map);

  void enqueue(ReferenceHashMap& map, oopDesc* from, oopDesc* to);

  static ByteSize byte_offset_of_buf() {
    return byte_offset_of(G1OopQueue, _buffer);
  }

  static ByteSize byte_offset_of_index() {
    return byte_offset_of(G1OopQueue, _index);
  }

  static void flush_all();

  static size_t byte_index_to_index(size_t ind) {
    assert(is_aligned(ind, _element_size), "precondition");
    return ind / _element_size;
  }

  static size_t index_to_byte_index(size_t ind) {
    return ind * _element_size;
  }

  size_t index() const {
    return byte_index_to_index(_index);
  }

  void set_index(size_t new_index) {
    assert(new_index <= capacity(), "precondition");
    _index = index_to_byte_index(new_index);
  }
};

#endif // SHARE_GC_G1_G1SATBMARKQUEUESET_HPP
