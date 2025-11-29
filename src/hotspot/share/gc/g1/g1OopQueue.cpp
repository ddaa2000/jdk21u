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
#include "gc/g1/g1ThreadLocalData.hpp"
#include "gc/g1/g1CollectedHeap.hpp"
#include "gc/g1/g1CollectedHeap.inline.hpp"
#include "oops/oop.inline.hpp"
#include "logging/log.hpp"

G1OopQueue::G1OopQueue() :
  _buffer(nullptr),
  _index(0),
  _total_count(0), _young_count(0), _identical_count(0) {
    _buffer = NEW_C_HEAP_ARRAY(oopDesc*, G1OopBufferSize * 2, mtGC);
    set_index(G1OopBufferSize * 2);
  }

G1OopQueue::~G1OopQueue(){
  FREE_C_HEAP_ARRAY(oopDesc*, _buffer);
  log_info(gc)("G1OopQueue: total_count: %zu, young_percent: %lf, identical_percent: %lf", 
              _total_count,
              _young_count * 100.0 / _total_count,
              _identical_count * 100.0 / (_total_count - _young_count));
}

void G1OopQueue::flush(ReferenceHashMap& map) {
  G1CollectedHeap* g1h = G1CollectedHeap::heap();
  size_t idx = index();
  Symbol* pre_from = nullptr;
  Symbol* pre_to = nullptr;
  size_t pre_count = 0;
  size_t pre_size = 0;
  for(size_t i = idx; i < G1OopBufferSize * 2; i += 2 ){
    oopDesc* from = _buffer[i];
    oopDesc* to = _buffer[i + 1];
    _total_count += 1;
    HeapRegion* from_region = g1h->heap_region_containing(from);
    HeapRegion* to_region = g1h->heap_region_containing(to);
    if(from_region->is_young() || to_region->is_young()){
      _young_count += 1;
      continue;
    }
    if(from_region->data_structure() != nullptr && !from_region->data_structure()->is_violated()){
      from_region->data_structure()->set_violated(true);
      g1h->inc_ds_violated_count();
    }



    if(pre_from == from->klass()->name() && pre_to == to->klass()->name()){
      pre_count++;
      pre_size += to->size();
      _identical_count += 1;
    } else {
      if(pre_from != nullptr){
        map.add_or_inc(pre_from, pre_to, pre_count, pre_size);
      }
      pre_from = from->klass()->name();
      pre_to = to->klass()->name();
      pre_count = 1;
      pre_size = to->size();
    }
    // map.add_or_inc(from->klass()->name(), to->klass()->name(), 1, to->size());
  }
  set_index(G1OopBufferSize * 2);
}

void G1OopQueue::enqueue(ReferenceHashMap& map, oopDesc* from, oopDesc* to){
  size_t idx = index();
  if(idx == 0){
    flush(map);
    idx = index();
  }
  idx -= 2;
  _buffer[idx] = from;
  _buffer[idx + 1] = to;
  set_index(idx);
}

void G1OopQueue::flush_all(){
  for (JavaThreadIteratorWithHandle jtiwh; JavaThread *thread = jtiwh.next();) {
    G1OopQueue& queue = G1ThreadLocalData::ref_queue(thread);
    ReferenceHashMap& map = G1ThreadLocalData::reference_hash_map(thread);
    queue.flush(map);
  }
}