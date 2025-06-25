/*
 * Copyright (c) 2001, 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_GC_G1_G1DATASTRUCTUREREGIONSET_INLINE_HPP
#define SHARE_GC_G1_G1DATASTRUCTUREREGIONSET_INLINE_HPP

#include "gc/g1/g1DataStructureRegionSet.hpp"
#include "gc/g1/heapRegion.hpp"
#include "gc/g1/g1Allocator.hpp"
#include "gc/g1/g1CollectedHeap.hpp"
#include "gc/g1/g1CollectedHeap.inline.hpp"
#include "gc/g1/g1CardTable.hpp"
#include "gc/g1/g1CardTable.inline.hpp"
#include "utilities/linkedlist.hpp"
#include "oops/symbolHandle.hpp"
#include "runtime/mutexLocker.hpp"


class DSChunkScanner {
    using Word = size_t;

    G1CardTable::CardValue* const _start_card;
    G1CardTable::CardValue* const _end_card;

    static const size_t ExpandedToScanMask = G1CardTable::WordAlreadyScanned;
    static const size_t ToScanMask = G1CardTable::g1_card_already_scanned;

    static bool is_card_dirty(const G1CardTable::CardValue* const card) {
      return (*card & ToScanMask) == 0;
    }

    static bool is_word_aligned(const void* const addr) {
      return ((uintptr_t)addr) % sizeof(Word) == 0;
    }

    G1CardTable::CardValue* find_first_dirty_card(G1CardTable::CardValue* i_card) const {
      while (!is_word_aligned(i_card)) {
        if (is_card_dirty(i_card)) {
          return i_card;
        }
        i_card++;
      }

      for (/* empty */; i_card < _end_card; i_card += sizeof(Word)) {
        Word word_value = *reinterpret_cast<Word*>(i_card);
        bool has_dirty_cards_in_word = (~word_value & ExpandedToScanMask) != 0;

        if (has_dirty_cards_in_word) {
          for (uint i = 0; i < sizeof(Word); ++i) {
            if (is_card_dirty(i_card)) {
              return i_card;
            }
            i_card++;
          }
          assert(false, "should have early-returned");
        }
      }

      return _end_card;
    }

    G1CardTable::CardValue* find_first_non_dirty_card(G1CardTable::CardValue* i_card) const {
      while (!is_word_aligned(i_card)) {
        if (!is_card_dirty(i_card)) {
          return i_card;
        }
        i_card++;
      }

      for (/* empty */; i_card < _end_card; i_card += sizeof(Word)) {
        Word word_value = *reinterpret_cast<Word*>(i_card);
        bool all_cards_dirty = (word_value == G1CardTable::WordAllDirty);

        if (!all_cards_dirty) {
          for (uint i = 0; i < sizeof(Word); ++i) {
            if (!is_card_dirty(i_card)) {
              return i_card;
            }
            i_card++;
          }
          assert(false, "should have early-returned");
        }
      }

      return _end_card;
    }

  public:
    DSChunkScanner(G1CardTable::CardValue* const start_card, G1CardTable::CardValue* const end_card) :
      _start_card(start_card),
      _end_card(end_card) {
        assert(is_word_aligned(start_card), "precondition");
        assert(is_word_aligned(end_card), "precondition");
      }

    template<typename Func>
    void on_dirty_cards(HeapRegion* hr, Func&& f) {
      for (G1CardTable::CardValue* cur_card = _start_card; cur_card < _end_card; /* empty */) {
        G1CardTable::CardValue* dirty_l = find_first_dirty_card(cur_card);
        G1CardTable::CardValue* dirty_r = find_first_non_dirty_card(dirty_l);

        assert(dirty_l <= dirty_r, "inv");

        if (dirty_l == dirty_r) {
          assert(dirty_r == _end_card, "finished the entire chunk");
          return;
        }

        f(hr, dirty_l, dirty_r);

        cur_card = dirty_r + 1;
        }
    }
};



template<typename Func>
void G1DataStructureRegionSet::scan_cards(Func&& f){
    MutexLocker ml(&_regions_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<HeapRegion*>* p = _regions.head();
    while (p != nullptr) {
        HeapRegion* region = *p->data();
        // log_info(gc)("remove data structure region %p  from data structure %p", region, this);
        G1CardTable::CardValue* left = _card_table->byte_for(region->bottom()), *right = _card_table->byte_for(region->end());
        DSChunkScanner scanner(left, right);
        scanner.on_dirty_cards(region, std::forward<Func>(f));
        scanner.on_dirty_cards(region, [&](HeapRegion* hr, G1CardTable::CardValue* left, G1CardTable::CardValue* right){
            _card_table->change_dirty_cards_to(left, right, G1CardTable::clean_card_val());
        });
        p = p->next();
    }
}

// template<typename Func>
// void G1DataStructureRegionSet::scan_cards(Func&& f){
//     HeapRegion* present_region = nullptr;
//     LinkedListNode<G1CardTable::CardValue*>* p = _out_cards.head();
//     G1CardTable::CardValue* left = nullptr, *right = nullptr;

//     G1CollectedHeap* g1h = G1CollectedHeap::heap();
//     G1CardTable* ct = g1h->card_table();

//     if(p != nullptr){
//         left = *p->data();
//         right = *p->data();
//         present_region = g1h->heap_region_containing(ct->addr_for(left));
//         assert(present_region->data_structure() == this, "must be");
//         // log_info(gc)("card is %p to %p", ct->addr_for(*p->data()), ct->addr_for(*p->data() + 1));
//     } else {
//         return;
//     }
    
//     while (p != nullptr) {
//         // log_info(gc)("card is %p to %p", ct->addr_for(*p->data()), ct->addr_for(*p->data() + 1));
//         G1CardTable::CardValue* present = *p->data();
//         HeapRegion* region = g1h->heap_region_containing(ct->addr_for(present));
//         assert(region->data_structure() == this, "must be");
//         if(present == right){
//             p = p->next();
//             continue;
//         }
//         if(region != present_region || present - right != 1){
//             f(present_region, left, right + 1);
//             left = present;
//             right = present;
//             present_region = region;
//         } else {
//             right = present;
//         }
//         p = p->next();
//     }
//     // if(!MemRegion(present_region->bottom(), present_region->top()).contains(MemRegion(ct->addr_for(left), ct->addr_for(right + 1)))){
//     //     log_info(gc)("%p %p %p %p", present_region->bottom(), present_region->top(), ct->addr_for(left), ct->addr_for(right + 1));
//     // }
//     f(present_region, left, right + 1);
// }

#endif // SHARE_GC_G1_G1DATASTRUCTUREREGIONSET_INLINE_HPP
 