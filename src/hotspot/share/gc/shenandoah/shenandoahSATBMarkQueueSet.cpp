/*
 * Copyright (c) 2018, 2019, Red Hat, Inc. All rights reserved.
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

#include "gc/shenandoah/shenandoahHeap.inline.hpp"
#include "gc/shenandoah/shenandoahSATBMarkQueueSet.hpp"
#include "gc/shenandoah/shenandoahThreadLocalData.hpp"

ShenandoahSATBMarkQueueSet::ShenandoahSATBMarkQueueSet(BufferNode::Allocator* allocator) :
  SATBMarkQueueSet(allocator)
{}

SATBMarkQueue& ShenandoahSATBMarkQueueSet::satb_queue_for_thread(Thread* const t) const {
  return ShenandoahThreadLocalData::satb_mark_queue(t);
}

class ShenandoahSATBMarkQueueFilterFn {
  ShenandoahHeap* const _heap;

public:
  ShenandoahSATBMarkQueueFilterFn(ShenandoahHeap* heap) : _heap(heap) {}

  // Return true if entry should be filtered out (removed), false if
  // it should be retained.
  bool operator()(const void* entry) const {
    return !_heap->requires_marking(entry);
  }
};

void ShenandoahSATBMarkQueueSet::filter(SATBMarkQueue& queue) {
  ShenandoahHeap* heap = ShenandoahHeap::heap();
  apply_filter(ShenandoahSATBMarkQueueFilterFn(heap), queue);
}










// Haoran: modify

ShenandoahPrefetchQueueSet::ShenandoahPrefetchQueueSet() : _heap(NULL) {}

void ShenandoahPrefetchQueueSet::initialize(ShenandoahHeap* heap,
                                    Monitor* cbl_mon,
                                    BufferNode::Allocator* allocator
                                    /*size_t process_completed_buffers_threshold,
                                    uint buffer_enqueue_threshold_percentage,
                                    Mutex* lock*/) {
  PrefetchQueueSet::initialize(cbl_mon,
                               allocator/*,
                               process_completed_buffers_threshold,
                               buffer_enqueue_threshold_percentage,
                               lock*/);
  _heap = heap;
}

void ShenandoahPrefetchQueueSet::handle_zero_index_for_thread(JavaThread* t) {
  ShenandoahThreadLocalData::prefetch_queue(t).handle_zero_index();
}

PrefetchQueue& ShenandoahPrefetchQueueSet::prefetch_queue_for_thread(JavaThread* const t) const{
  return ShenandoahThreadLocalData::prefetch_queue(t);
}

// Should enqueue or not?
// Should enqueue if it is in the heap

static inline bool requires_marking_prefetch(const void* entry, ShenandoahHeap* heap) {

  if(heap->is_in_reserved(entry)) {
    return true;
  }
  return false;
  // // Includes rejection of NULL pointers.
  // assert(g1h->is_in_reserved(entry),
  //        "Non-heap pointer in SATB buffer: " PTR_FORMAT, p2i(entry));

  // HeapRegion* region = g1h->heap_region_containing(entry);
  // assert(region != NULL, "No region for " PTR_FORMAT, p2i(entry));
  // if (entry >= region->next_top_at_mark_start()) {
  //   return false;
  // }

  // assert(oopDesc::is_oop(oop(entry), true /* ignore mark word */),
  //        "Invalid oop in SATB buffer: " PTR_FORMAT, p2i(entry));

  // return true;
}

static inline bool discard_entry_prefetch(const void* entry, ShenandoahHeap* heap) {
  return !requires_marking_prefetch(entry, heap) || heap->marking_context()->is_marked((oop)entry);
}

// Workaround for not yet having std::bind.
class ShenandoahPrefetchQueueFilterFn {
  ShenandoahHeap* _heap;

public:
  ShenandoahPrefetchQueueFilterFn(ShenandoahHeap* heap) : _heap(heap) {}

  // Return true if entry should be filtered out (removed), false if
  // it should be retained.
  bool operator()(const void* entry) const {
    return discard_entry_prefetch(entry, _heap);
  }
};

void ShenandoahPrefetchQueueSet::filter(PrefetchQueue* queue) {
  assert(_heap != NULL, "SATB queue set not initialized");
  apply_filter(ShenandoahPrefetchQueueFilterFn(_heap), queue);
}
