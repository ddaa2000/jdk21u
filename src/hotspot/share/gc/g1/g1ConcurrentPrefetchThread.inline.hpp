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

#ifndef SHARE_VM_GC_G1_G1CONCURRENTPREFETCHTHREAD_INLINE_HPP
#define SHARE_VM_GC_G1_G1CONCURRENTPREFETCHTHREAD_INLINE_HPP

#include "gc/g1/g1ConcurrentPrefetch.hpp"
#include "gc/g1/g1ConcurrentPrefetchThread.hpp"

// // Total virtual time so far.
// inline double G1ConcurrentPrefetchThread::vtime_accum() {
//   return _vtime_accum + _pf->all_task_accum_vtime();
// }

// // Marking virtual time so far
// inline double G1ConcurrentPrefetchThread::vtime_mark_accum() {
//   return _vtime_mark_accum + _pf->all_task_accum_vtime();
// }

inline void G1ConcurrentPrefetchThread::start_full_mark() {
  assert(_state == Idle, "cycle in progress");
  _state = FullMark;
}

inline void G1ConcurrentPrefetchThread::start_undo_mark() {
  assert(_state == Idle, "cycle in progress");
  _state = UndoMark;
}

inline bool G1ConcurrentPrefetchThread::idle() const { return _state == Idle; }

inline bool G1ConcurrentPrefetchThread::in_progress() const {
  return !idle();
}

inline bool G1ConcurrentPrefetchThread::in_undo_mark() const {
  return _state == UndoMark;
}

inline void G1ConcurrentPrefetchThread::set_idle() {
  assert(_state == FullMark || _state == UndoMark, "must not be starting a new cycle");
  _state = Idle;
}

#endif // SHARE_VM_GC_G1_G1CONCURRENTMARKTHREAD_INLINE_HPP
