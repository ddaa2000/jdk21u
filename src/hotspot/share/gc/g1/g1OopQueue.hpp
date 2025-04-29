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

#ifndef SHARE_GC_G1_G1OOPQUEUESET_HPP
#define SHARE_GC_G1_G1OOPQUEUESET_HPP

#include "oops/oop.hpp"


class G1OopQueue : CHeapObj<mtGC> {
private:
  oopDesc** _buffer;
  size_t _index;
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

  static void flush_all(){
    for (JavaThreadIteratorWithHandle jtiwh; JavaThread *thread = jtiwh.next();) {
      G1OopQueue& queue = G1ThreadLocalData::ref_queue(thread);
      queue.flush();
    }
  }
};

#endif // SHARE_GC_G1_G1SATBMARKQUEUESET_HPP
