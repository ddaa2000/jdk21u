/*
 * Copyright (c) 2001, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_GC_G1_G1CONCURRENTPREFETCH_INLINE_HPP
#define SHARE_VM_GC_G1_G1CONCURRENTPREFETCH_INLINE_HPP

#include "gc/g1/g1CollectedHeap.inline.hpp"
#include "gc/g1/g1ConcurrentMark.hpp"
#include "gc/g1/g1ConcurrentPrefetch.hpp"
#include "gc/g1/g1ConcurrentMarkBitMap.inline.hpp"
#include "gc/g1/g1ConcurrentMarkObjArrayProcessor.inline.hpp"
#include "gc/g1/g1OopClosures.inline.hpp"
#include "gc/g1/g1Policy.hpp"
#include "gc/g1/g1RegionMarkStatsCache.inline.hpp"
#include "gc/g1/g1RemSetTrackingPolicy.hpp"
#include "gc/g1/heapRegionRemSet.hpp"
#include "gc/g1/heapRegion.hpp"
#include "gc/shared/suspendibleThreadSet.hpp"
#include "gc/shared/taskqueue.inline.hpp"
#include "utilities/bitMap.inline.hpp"
#include "runtime/orderAccess.hpp"




inline bool G1ConcurrentPrefetch::mark_in_bitmap(uint const worker_id, oop const obj) {
  HeapRegion* const hr = _g1h->heap_region_containing(obj);
  return mark_in_bitmap(worker_id, hr, obj);
}

inline bool G1ConcurrentPrefetch::mark_in_bitmap(uint const worker_id, HeapRegion* const hr, oop const obj) {
  assert(hr != NULL, "just checking");
  assert(hr->is_in_reserved(obj), "Attempting to mark object at " PTR_FORMAT " that is not contained in the given region %u", p2i(obj), hr->hrm_index());

  if (hr->obj_allocated_since_marking_start(obj)) {
    return false;
  }

  // Some callers may have stale objects to mark above nTAMS after humongous reclaim.
  // Can't assert that this is a valid object at this point, since it might be in the process of being copied by another thread.
  assert(!hr->is_continues_humongous(), "Should not try to mark object " PTR_FORMAT " in Humongous continues region %u above nTAMS " PTR_FORMAT, p2i(obj), hr->hrm_index(), p2i(hr->top_at_mark_start()));

  // HeapWord* const obj_addr = (HeapWord*)obj;

  bool success = _cm->_mark_bitmap.par_mark(obj);
  if (success) {
    add_to_liveness(worker_id, obj, obj->size());
  }
  return success;
}

//hua: returns true if the objects needs to be marked black. False if it is already black or there is no need.
inline bool G1ConcurrentPrefetch::mark_black_in_bitmap(uint const worker_id, oop const obj) {
  HeapRegion* const hr = _g1h->heap_region_containing(obj);
  assert(hr != NULL, "just checking");
  assert(hr->is_in_reserved(obj), "Attempting to mark object at " PTR_FORMAT " that is not contained in the given region %u", p2i(obj), hr->hrm_index());

  if (hr->obj_allocated_since_marking_start(obj)) {
    return false;
  }

  // Some callers may have stale objects to mark above nTAMS after humongous reclaim.
  // Can't assert that this is a valid object at this point, since it might be in the process of being copied by another thread.
  assert(!hr->is_continues_humongous(), "Should not try to mark object " PTR_FORMAT " in Humongous continues region %u above nTAMS " PTR_FORMAT, p2i(obj), hr->hrm_index(), p2i(hr->top_at_mark_start()));

  // HeapWord* const obj_addr = (HeapWord*)obj;

  //For the objects in the prefetech queue
  //  if the object is already marked black -> do nothing
  //  else if the object is marked grey -> mark black, ensure its children put to the local stack
  //  else (the object is not marked) -> mark black, ensure its children put to the local stack


  bool success = _cm->_mark_bitmap.par_mark(obj);
  if (success) {
    add_to_liveness(worker_id, obj, obj->size());
  }

  // if (!success && is_below_global_finger(obj)){ // already black
  //   return false;
  // }
  OrderAccess::storestore();
  // now the object is at least grey
  // if(success){
  //   bool success_black = _cm->_mark_black_bitmap.par_mark(obj);
  // }
  
  // return success_black;

  return success;
}

// #ifndef PRODUCT
// template<typename Fn>
// inline void G1CMMarkStack::iterate(Fn fn) const {
//   assert_at_safepoint_on_vm_thread();

//   size_t num_chunks = 0;

//   TaskQueueEntryChunk* cur = _chunk_list;
//   while (cur != NULL) {
//     guarantee(num_chunks <= _chunks_in_chunk_list, "Found " SIZE_FORMAT " oop chunks which is more than there should be", num_chunks);

//     for (size_t i = 0; i < EntriesPerChunk; ++i) {
//       if (cur->data[i].is_null()) {
//         break;
//       }
//       fn(cur->data[i]);
//     }
//     cur = cur->next;
//     num_chunks++;
//   }
// }
// #endif

// It scans an object and visits its children.
inline void G1PFTask::scan_task_entry(G1TaskQueueEntry task_entry) { process_grey_task_entry<true>(task_entry); }

inline void G1PFTask::push(G1TaskQueueEntry task_entry) {
  // assert(task_entry.is_array_slice() || _g1h->is_in_reserved(task_entry.obj()), "invariant");
  // assert(task_entry.is_array_slice() || !_g1h->is_on_master_free_list(
  //             _g1h->heap_region_containing(task_entry.obj())), "invariant");
  // assert(task_entry.is_array_slice() || !_g1h->is_obj_ill(task_entry.obj()), "invariant");  // FIXME!!!
  // assert(task_entry.is_array_slice() || _mark_bitmap->is_marked((HeapWord*)task_entry.obj()), "invariant");

  assert(task_entry.is_array_slice() || _g1h->is_in_reserved(task_entry.obj()), "invariant");
  assert(task_entry.is_array_slice() || !_g1h->is_on_master_free_list(
              _g1h->heap_region_containing(task_entry.obj())), "invariant");
  assert(task_entry.is_array_slice() || _mark_bitmap->is_marked(cast_from_oop<HeapWord*>(task_entry.obj())), "invariant");


  if (!_task_queue->push(task_entry)) {

    // The local task queue looks full. We need to push some entries
    // to the global stack.
    // ShouldNotReachHere();
    move_entries_to_global_stack();

    // this should succeed since, even if we overflow the global
    // stack, we should have definitely removed some entries from the
    // local queue. So, there must be space on it.
    bool success = _task_queue->push(task_entry);
    assert(success, "invariant");

    // ShouldNotReachHere();
  }
}



template<bool scan>
inline void G1PFTask::process_grey_task_entry(G1TaskQueueEntry task_entry) {
  assert(scan || (task_entry.is_oop() && task_entry.obj()->is_typeArray()), "Skipping scan of grey non-typeArray");
  assert(task_entry.is_array_slice() || _mark_bitmap->is_marked(cast_from_oop<HeapWord*>(task_entry.obj())),
         "Any stolen object should be a slice or marked");

  if (scan) {
    if (task_entry.is_array_slice()) {
      _words_scanned += _objArray_processor.process_slice(task_entry.slice());
      _objs_scanned ++;
    } else {
      oop obj = task_entry.obj();
      obj = cast_to_oop(cast_from_oop<uintptr_t>(obj) & ((((uintptr_t)1)<<63)-1));
      if (G1PFObjArrayProcessor::should_be_sliced(obj)) {
        _words_scanned += _objArray_processor.process_obj(obj);
        _objs_scanned ++;
      } else {
        _words_scanned += obj->oop_iterate_size(_cm_oop_closure);
        _objs_scanned ++;
        // bool success_black = _cm->_mark_black_bitmap.par_mark(obj);
      }
    }
  } 
  // else {
  //   oop obj = task_entry.obj();
  //   bool success_black = _cm->_mark_black_bitmap.par_mark(obj);
  // }
//   check_limits();
}

inline size_t G1PFTask::scan_objArray(objArrayOop obj, MemRegion mr) {
  obj->oop_iterate(_cm_oop_closure, mr);
  return mr.word_size();
}



inline void G1PFTask::update_liveness(oop const obj, const size_t obj_size) {
  _mark_stats_cache.add_live_words(_g1h->addr_to_region(obj), obj_size);
}

inline void G1ConcurrentPrefetch::add_to_liveness(uint worker_id, oop const obj, size_t size) {
  task(worker_id)->update_liveness(obj, size);
}

// inline void G1CMTask::abort_marking_if_regular_check_fail() {
//   if (!regular_clock_call()) {
//     set_has_aborted();
//   }
// }

inline bool G1PFTask::is_below_global_finger(oop obj) const {
  // If obj is above the global finger, then the mark bitmap scan
  // will find it later, and no push is needed.  Similarly, if we have
  // a current region and obj is between the local finger and the
  // end of the current region, then no push is needed.  The tradeoff
  // of checking both vs only checking the global finger is that the
  // local check will be more accurate and so result in fewer pushes,
  // but may also be a little slower.
  return _pf->is_below_global_finger(obj);
  // HeapWord* global_finger = _cm->finger();
  // HeapWord* objAddr = cast_from_oop<HeapWord*>(obj);
  // return objAddr < global_finger;
}

inline bool G1ConcurrentPrefetch::is_below_global_finger(oop obj) const {
  // If obj is above the global finger, then the mark bitmap scan
  // will find it later, and no push is needed.  Similarly, if we have
  // a current region and obj is between the local finger and the
  // end of the current region, then no push is needed.  The tradeoff
  // of checking both vs only checking the global finger is that the
  // local check will be more accurate and so result in fewer pushes,
  // but may also be a little slower.
  HeapWord* global_finger = _cm->finger();
  HeapWord* objAddr = cast_from_oop<HeapWord*>(obj);
  return objAddr < global_finger;
}

inline bool G1PFTask::make_reference_grey(oop obj) {
  if (!_pf->mark_in_bitmap(_worker_id, obj)) {
    return false;
  }
    G1TaskQueueEntry entry = G1TaskQueueEntry::from_oop(obj);
    if (obj->is_typeArray()) {
        // Immediately process arrays of primitive types, rather
        // than pushing on the mark stack.  This keeps us from
        // adding humongous objects to the mark stack that might
        // be reclaimed before the entry is processed - see
        // selection of candidates for eager reclaim of humongous
        // objects.  The cost of the additional type test is
        // mitigated by avoiding a trip through the mark stack,
        // by only doing a bookkeeping update and avoiding the
        // actual scan of the object - a typeArray contains no
        // references, and the metadata is built-in.
        process_grey_task_entry<false>(entry);
    } else {
        push(entry);
    }
  return true;
}

inline bool G1PFTask::make_reference_black(oop obj) {
  if (!_pf->mark_in_bitmap(_worker_id, obj)) {
    return false;
  }

  G1TaskQueueEntry entry = G1TaskQueueEntry::from_oop(obj);
  if (obj->is_typeArray()) {
      // Immediately process arrays of primitive types, rather
      // than pushing on the mark stack.  This keeps us from
      // adding humongous objects to the mark stack that might
      // be reclaimed before the entry is processed - see
      // selection of candidates for eager reclaim of humongous
      // objects.  The cost of the additional type test is
      // mitigated by avoiding a trip through the mark stack,
      // by only doing a bookkeeping update and avoiding the
      // actual scan of the object - a typeArray contains no
      // references, and the metadata is built-in.
      process_grey_task_entry<false>(entry);
  } else {
      push(entry);
  }

  return true;
}


template <class T>
inline bool G1PFTask::deal_with_reference(T* p) {
  // increment_refs_reached();
  oop const obj = RawAccess<MO_RELAXED>::oop_load(p);
  if (obj == NULL) {
    return false;
  }
  // return make_reference_grey(obj);
  return make_reference_black(obj);

}

// inline void G1ConcurrentPrefetch::mark_in_prev_bitmap(oop p) {
// //   assert(!_prev_mark_bitmap->is_marked((HeapWord*) p), "sanity");
//   _cm->_prev_mark_bitmap->mark((HeapWord*) p);
// }

// bool G1ConcurrentPrefetch::is_marked_in_prev_bitmap(oop p) const {
//   assert(p != NULL && oopDesc::is_oop(p), "expected an oop");
//   return _cm->prev_mark_bitmap()->is_marked((HeapWord*)p);
// }

// bool G1ConcurrentPrefetch::is_marked_in_next_bitmap(oop p) const {
//   assert(p != NULL && oopDesc::is_oop(p), "expected an oop");
//   return _cm->next_mark_bitmap()->is_marked((HeapWord*)p);
// }

inline bool G1ConcurrentPrefetch::do_yield_check() {
  if (SuspendibleThreadSet::should_yield()) {
    SuspendibleThreadSet::yield();
    return true;
  } else {
    return false;
  }
}

#endif // SHARE_VM_GC_G1_G1CONCURRENTMARK_INLINE_HPP
