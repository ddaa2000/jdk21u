/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates. All rights reserved.
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

#include "precompiled.hpp"
#include "gc/g1/g1OopQueue.hpp"
#include "gc/g1/heapRegion.hpp"
#include "gc/shared/satbMarkQueue.hpp"
#include "oops/oop.hpp"
#include "utilities/debug.hpp"
#include "utilities/globalDefinitions.hpp"

G1OopQueue::G1OopQueue() :
  _buffer(nullptr),
  _index(G1OopBufferSize * 2),
{
  _buffer = NEW_C_HEAP_ARRAY(oopDesc*, G1OopBufferSize * 2, mtGC);
}

G1OopQueue::~G1OopQueue() : {
  delete[] _buffer;
}

void G1OopQueue::flush(ReferenceHashMap& map) {
  for(size_t i = G1OopBufferSize * 2 - 2; i >= _index; i -= 2 ){
    oopDesc* from = _buffer[i];
    oopDesc* to = _buffer[i + 1];
    map.add_or_inc(from->klass()->name(), to->klass()->name(), 1, to->size());
  }
  _index = G1OopBufferSize * 2;
}

void G1OopQueue::enqueue(ReferenceHashMap& map, oopDesc* from, oopDesc* to){
  if(_index == 0){
    flush(map);
  }
  _index -= 2;
  buffer[_index] = from;
  buffer[_index + 1] = to;
}

