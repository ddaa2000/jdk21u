#include "gc/g1/g1DataStructureRegionSet.hpp"
#include "gc/g1/g1DataStructureManager.hpp"
#include "gc/g1/heapRegion.hpp"
#include "gc/g1/g1CollectedHeap.hpp"
#include "gc/g1/g1CollectedHeap.inline.hpp"
#include "gc/shared/plab.hpp"
#include "oops/oop.hpp"
#include "oops/oop.inline.hpp"
#include "oops/klass.hpp"
#include "oops/symbol.hpp"
#include "classfile/symbolTable.hpp"


G1DataStructure* G1DataStructureManager::get_data_structure_by_root(Symbol* root_symbol) {
    G1DataStructure* data_structure = nullptr;
    LinkedListNode<G1DataStructure*>* p = _data_structure_types.head();
    while (p != nullptr) {
        data_structure = *p->data();
        // log_info(gc)("data structure");
        if (data_structure->symbol_in_roots(root_symbol) != nullptr) {
            return data_structure;
        }
        p = p->next();
    }
    return nullptr;
}

// G1DataStructureRegionSet* G1DataStructureManager::get_data_structure(oop from_oop, oop to_oop) {
//     G1CollectedHeap* g1h = G1CollectedHeap::heap();
//     Symbol* to_symbol = to_oop->klass()->name();
//     G1DataStructureRegionSet* data_structure = nullptr;
//     G1DataStructure* data_structure_type = nullptr;

//     static Symbol* l_object = SymbolTable::new_symbol("[Ljava/lang/Object;");
//     static Symbol* l_compact_buffer = SymbolTable::new_symbol("[Lorg/apache/spark/util/collection/CompactBuffer;");
//     static Symbol* tuple2 = SymbolTable::new_symbol("scala/Tuple2");
//     static Symbol* tuple2_mcII_sp = SymbolTable::new_symbol("scala/Tuple2$mcII$sp");
//     static Symbol* l_tuple2 = SymbolTable::new_symbol("[Lscala/Tuple2;");

//     if (from_oop != nullptr) {
//         Symbol* from_symbol = from_oop->klass()->name();
//         Symbol* to_symbol = to_oop->klass()->name();
//         HeapRegion* from_region = g1h->heap_region_containing(from_oop);



//         if (from_region->data_structure() != nullptr) {
//             G1DataStructureRegionSet* data_structure = from_region->data_structure();
//             if (data_structure->find_edge(from_symbol, to_symbol) != nullptr) {
//                 // log_info(gc)("found %s to %s", from_symbol->as_C_string(), to_symbol->as_C_string());
//                 return data_structure;
//             } else {
//                 // log_info(gc)("not found %s to %s", from_symbol->as_C_string(), to_symbol->as_C_string());
//             }
//         } else if(from_region->is_humongous()){
//             // log_info(gc)("humongous region %u", from_region->hrm_index());
//             data_structure_type = get_data_structure_by_root(from_symbol);
//             if (data_structure_type == nullptr) {
//                 // log_info(gc)("humongous region %u not root", from_region->hrm_index());
//                 bool l_object_wanted = from_oop->klass()->name() == l_object && (to_symbol == tuple2 || to_symbol == l_compact_buffer || to_symbol == tuple2_mcII_sp);
//                 if(l_object_wanted || from_symbol == l_tuple2 || from_symbol == l_compact_buffer) {
//                         // log_info(gc)("humongous region %u is tuple2 /or compact", from_region->hrm_index());
//                         LinkedListNode<G1DataStructure*>* p = _data_structure_types.head();
//                         data_structure_type = *p->data();
//                         if(data_structure_type == nullptr){
//                             ShouldNotReachHere();
//                         }
//                     // } 
//                     // else if( || ){
//                     //     log_info(gc)("humongous region %u child %s", from_region->hrm_index(), to_symbol->as_C_string());
//                     // }
//                 }
//             }
//             if(data_structure_type != nullptr){
//                 MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
//                 OrderAccess::storestore();
//                 if (from_region->data_structure() == nullptr) {
//                     data_structure = new G1DataStructureRegionSet(g1h, data_structure_type, _present_id);
//                     if(_allocator == nullptr || _evacuation_info == nullptr) {
//                         ShouldNotReachHere();
//                     }

//                     {
//                         log_info(gc)("create data structure for obj %p, class %s, at %p, id %u", to_oop, from_symbol->as_C_string(), data_structure, _present_id);
//                         // log_info(gc)("create data structure for obj %p, class %s", to_oop, from_symbol->as_C_string());
//                         data_structure->init_data_structure_alloc_region(_allocator, _evacuation_info);
//                         _data_structures.add(data_structure);
//                         _present_id++;
//                     }

//                     data_structure->add_region(from_region);
//                     from_region->set_data_structure(data_structure);
//                     from_region->set_collect_as_a_whole(true);


//                     for(uint i = from_region->hrm_index() + 1; i < g1h->max_regions(); i++){
//                         HeapRegion* hr = g1h->region_at_or_null(i);
//                         if(hr == nullptr || !hr->is_continues_humongous()){
//                             break;
//                         }
//                         data_structure->add_region(hr);
//                         hr->set_data_structure(data_structure);
//                         hr->set_collect_as_a_whole(true);
//                     }
//                     return data_structure;
//                 } else {
//                     data_structure = from_region->data_structure();
//                     if (data_structure->find_edge(from_symbol, to_symbol) != nullptr) {
//                         // log_info(gc)("found %s to %s", from_symbol->as_C_string(), to_symbol->as_C_string());
//                         return data_structure;
//                     }
//                 }
//             }
//         }
//         // else {
//         //     LinkedListNode<G1DataStructure*>* p = _data_structure_types.head();
//         //     data_structure_type = *p->data();
//         //     if(data_structure_type->find_edge(from_symbol, to_symbol) != nullptr) {
//         //         // log_info(gc)("found outer %s to %s", from_symbol->as_C_string(), to_symbol->as_C_string());
//         //     }
//         // }
//     }

//     data_structure_type = get_data_structure_by_root(to_symbol);
//     if (data_structure_type != nullptr) {
//         MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
//         data_structure = new G1DataStructureRegionSet(g1h, data_structure_type, _present_id);
//         if(_allocator == nullptr || _evacuation_info == nullptr) {
//             ShouldNotReachHere();
//         }

//         {
//             log_info(gc)("create normal data structure for obj %p, class %s, at %p, id %u, from class %s", to_oop, to_symbol->as_C_string(), data_structure, _present_id,
//                          from_oop != nullptr ? from_oop->klass()->name()->as_C_string() : "null");
//             data_structure->init_data_structure_alloc_region(_allocator, _evacuation_info);
//             _data_structures.add(data_structure);
//             _present_id++;
//             // log_info(gc)("after create normal");
//         }
//         return data_structure;
//     }



//     // log_info(gc)("found %s to %s", from_symbol->as_C_string(), to_symbol->as_C_string());

//     // if(to_oop->klass()->name() == l_object) {
//     //     objArrayOop obj_array = objArrayOop(to_oop);
//     //     // SymbolHandle handle(from_symbol);

//     //     if(obj_array->length() > 0 && obj_array->obj_at(0) != nullptr && 
//     //     (obj_array->obj_at(0)->klass()->name() == tuple2 || obj_array->obj_at(0)->klass()->name() == l_compact_buffer)) {
//     //         log_info(gc)("found tuple2 array from %s", from_oop != nullptr ? from_oop->klass()->name()->as_C_string() : "null");
//     //         MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
//     //         OrderAccess::storestore();
//     //         LinkedListNode<G1DataStructure*>* p = _data_structure_types.head();
//     //         data_structure_type = *p->data();
//     //         data_structure = new G1DataStructureRegionSet(g1h, data_structure_type, _present_id);
//     //         {
//     //             log_info(gc)("create data structure for obj %p, at %p, id %u", to_oop, data_structure, _present_id);
//     //             // log_info(gc)("create data structure for obj %p, class %s", to_oop, from_symbol->as_C_string());
//     //             data_structure->init_data_structure_alloc_region(_allocator, _evacuation_info);
//     //             _data_structures.add(data_structure);
//     //             _present_id++;
//     //         }
//     //         return data_structure;
//     //     }
//     // }

//     return nullptr;
// }

void G1DataStructureManager::check_add_humongous(oop from_oop, oop to_oop) {
    G1CollectedHeap* g1h = G1CollectedHeap::heap();
    HeapRegion* to_region = g1h->heap_region_containing(to_oop);
    if(to_region->data_structure() != nullptr){
        return;
    }
    log_info(gc)("check oop %p", to_oop);
    G1DataStructureRegionSet* ds = get_data_structure(from_oop, to_oop);
    if(ds != nullptr){
        log_info(gc)("before check oop mutex");
        // MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
        log_info(gc)("inside check oop mutex");
        OrderAccess::storestore();
        if (to_region->data_structure() == nullptr) {
            ds->add_region(to_region);
            to_region->set_data_structure(ds);
            to_region->set_collect_as_a_whole(true);


            for(uint i = to_region->hrm_index() + 1; i < g1h->max_regions(); i++){
                HeapRegion* hr = g1h->region_at_or_null(i);
                if(hr == nullptr || !hr->is_continues_humongous()){
                    break;
                }
                ds->add_region(hr);
                hr->set_data_structure(ds);
                hr->set_collect_as_a_whole(true);
            }
        }
    }
    log_info(gc)("after check oop mutex");

}

G1DataStructureRegionSet* G1DataStructureManager::get_data_structure(oop from_oop, oop to_oop) {
    G1CollectedHeap* g1h = G1CollectedHeap::heap();
    Symbol* to_symbol = to_oop->klass()->name();
    G1DataStructureRegionSet* data_structure = nullptr;
    G1DataStructure* data_structure_type = nullptr;

    if (from_oop != nullptr) {
        Symbol* from_symbol = from_oop->klass()->name();
        Symbol* to_symbol = to_oop->klass()->name();
        HeapRegion* from_region = g1h->heap_region_containing(from_oop);



        if (from_region->data_structure() != nullptr) {
            G1DataStructureRegionSet* data_structure = from_region->data_structure();
            if (data_structure->find_edge(from_symbol, to_symbol) != nullptr) {
                // log_info(gc)("found %s to %s", from_symbol->as_C_string(), to_symbol->as_C_string());
                return data_structure;
            } 
            // else {
            //     log_info(gc)("not found %s to %s", from_symbol->as_C_string(), to_symbol->as_C_string());
            // }
        } else if(from_region->is_humongous()){
            // log_info(gc)("humongous region %u", from_region->hrm_index());
            data_structure_type = get_data_structure_by_root(from_symbol);

            if(data_structure_type != nullptr){
                MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
                OrderAccess::storestore();
                if (from_region->data_structure() == nullptr) {
                    data_structure = new G1DataStructureRegionSet(g1h, data_structure_type, _present_id);
                    if(_allocator == nullptr || _evacuation_info == nullptr) {
                        ShouldNotReachHere();
                    }

                    {
                        log_info(gc)("create data structure for obj %p, class %s, at %p, id %u", to_oop, from_symbol->as_C_string(), data_structure, _present_id);
                        // log_info(gc)("create data structure for obj %p, class %s", to_oop, from_symbol->as_C_string());
                        data_structure->init_data_structure_alloc_region(_allocator, _evacuation_info);
                        _data_structures.add(data_structure);
                        _present_id++;
                    }

                    data_structure->add_region(from_region);
                    from_region->set_data_structure(data_structure);
                    from_region->set_collect_as_a_whole(true);


                    for(uint i = from_region->hrm_index() + 1; i < g1h->max_regions(); i++){
                        HeapRegion* hr = g1h->region_at_or_null(i);
                        if(hr == nullptr || !hr->is_continues_humongous()){
                            break;
                        }
                        data_structure->add_region(hr);
                        hr->set_data_structure(data_structure);
                        hr->set_collect_as_a_whole(true);
                    }
                    return data_structure;
                } else {
                    data_structure = from_region->data_structure();
                    if (data_structure->find_edge(from_symbol, to_symbol) != nullptr) {
                        // log_info(gc)("found %s to %s", from_symbol->as_C_string(), to_symbol->as_C_string());
                        return data_structure;
                    }
                }
            }
        }
        // else {
        //     LinkedListNode<G1DataStructure*>* p = _data_structure_types.head();
        //     data_structure_type = *p->data();
        //     if(data_structure_type->find_edge(from_symbol, to_symbol) != nullptr) {
        //         // log_info(gc)("found outer %s to %s", from_symbol->as_C_string(), to_symbol->as_C_string());
        //     }
        // }
    }

    data_structure_type = get_data_structure_by_root(to_symbol);
    if (data_structure_type != nullptr) {
        MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
        data_structure = new G1DataStructureRegionSet(g1h, data_structure_type, _present_id);
        if(_allocator == nullptr || _evacuation_info == nullptr) {
            ShouldNotReachHere();
        }

        {
            log_info(gc)("create normal data structure for obj %p, class %s, at %p, id %u, from class %s", to_oop, to_symbol->as_C_string(), data_structure, _present_id,
                         from_oop != nullptr ? from_oop->klass()->name()->as_C_string() : "null");
            data_structure->init_data_structure_alloc_region(_allocator, _evacuation_info);
            _data_structures.add(data_structure);
            _present_id++;
        }
        return data_structure;
    }

    return nullptr;
}

void G1DataStructureManager::init_data_structure_alloc_regions(G1Allocator* allocator, G1EvacInfo* evacuation_info) {
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    _allocator = allocator;
    _evacuation_info = evacuation_info;
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        data_structure->init_data_structure_alloc_region(allocator, evacuation_info);
        p = p->next();
    }
}

uint G1DataStructureManager::alloc_count() {
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    uint count = 0;
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        count += data_structure->alloc_region()->count();
        p = p->next();
    }
    return count;
}

void G1DataStructureManager::release_data_structure_alloc_regions() {
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        data_structure->release_data_structure_alloc_region();
        p = p->next();
    }
}

void G1DataStructureManager::abandon_data_structure_alloc_regions() {
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        data_structure->abandon_alloc_region();
        p = p->next();
    }
}

bool G1DataStructureManager::is_retained_old_region(HeapRegion* hr) {
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        if (data_structure->region_in(hr)) {
            return data_structure->is_retained_old_region(hr);
        }
        p = p->next();
    }
    return false;
}

// void G1DataStructureManager::initialize_predefined_data_structures() {

//     // java/nio/HeapByteBuffer -> [B : weight_factor: 36.50% size_factor: 0.36499393036008876 weight: 1641539726, count: 24002
//     // org/apache/cassandra/db/rows/BufferCell -> java/nio/HeapByteBuffer : weight_factor: 90.24% size_factor: 0.3293804708265296 weight: 1017215136, count: 127151892
//     // [Ljava/lang/Object; -> org/apache/cassandra/db/rows/BufferCell : weight_factor: 100.00% size_factor: 0.3293804708265296 weight: 890089193, count: 127155599
//     // org/apache/cassandra/db/partitions/BTreePartitionData -> [Ljava/lang/Object; : weight_factor: 19.24% size_factor: 0.06337376136688198 weight: 42225579, count: 14075193
//     // org/apache/cassandra/db/rows/BTreeRow -> [Ljava/lang/Object; : weight_factor: 77.71% size_factor: 0.25595970092397613 weight: 170544502, count: 14075802
//     // org/apache/cassandra/db/partitions/AtomicBTreePartition -> org/apache/cassandra/db/partitions/BTreePartitionData : weight_factor: 100.00% size_factor: 0.06337068166005362 weight: 98521983, count: 14074569
//     // [Ljava/lang/Object; -> org/apache/cassandra/db/rows/BTreeRow : weight_factor: 100.00% size_factor: 0.25595940997401456 weight: 98530530, count: 14075790
//     // java/util/concurrent/ConcurrentSkipListMap$Node -> org/apache/cassandra/db/partitions/AtomicBTreePartition : weight_factor: 100.00% size_factor: 0.06337068166005362 weight: 81834102, count: 13639017
//     // java/util/concurrent/ConcurrentSkipListMap$Node -> java/util/concurrent/ConcurrentSkipListMap$Node : weight_factor: 86.38% size_factor: 0.05474268389025206 weight: 59450900, count: 11890180
//     // java/util/concurrent/ConcurrentSkipListMap$Index -> java/util/concurrent/ConcurrentSkipListMap$Node: 0.21%, weight: 9369905, count: 1873981
//     // java/util/concurrent/ConcurrentSkipListMap$Index -> java/util/concurrent/ConcurrentSkipListMap$Index: 1.63%, weight: 34335245, count: 6867049
//     // [Ljava/lang/Object; -> java/util/concurrent/ConcurrentSkipListMap$Index: 0.00%, weight: 615, count: 123
//     // java/util/concurrent/ConcurrentSkipListMap -> java/util/concurrent/ConcurrentSkipListMap$Index: 0.00%, weight: 580, count: 116


//     Symbol* skip_list_map = SymbolTable::new_symbol("java/util/concurrent/ConcurrentSkipListMap");
//     Symbol* skip_list_index = SymbolTable::new_symbol("java/util/concurrent/ConcurrentSkipListMap$Index");
//     Symbol* skip_list_node = SymbolTable::new_symbol("java/util/concurrent/ConcurrentSkipListMap$Node");
//     Symbol* btree_partition = SymbolTable::new_symbol("org/apache/cassandra/db/partitions/AtomicBTreePartition");
//     Symbol* btree_partition_data = SymbolTable::new_symbol("org/apache/cassandra/db/partitions/BTreePartitionData");
//     Symbol* l_object = SymbolTable::new_symbol("[Ljava/lang/Object;");
//     Symbol* btree_row = SymbolTable::new_symbol("org/apache/cassandra/db/rows/BTreeRow");
//     Symbol* buffer_cell = SymbolTable::new_symbol("org/apache/cassandra/db/rows/BufferCell");
//     Symbol* heap_byte_buffer = SymbolTable::new_symbol("java/nio/HeapByteBuffer");
//     Symbol* l_byte = SymbolTable::new_symbol("[B");

//     G1DataStructure* data_structure = new G1DataStructure();
//     // data_structure->add_root(skip_list_map);
//     data_structure->add_root(skip_list_index);
//     // data_structure->add_edge(skip_list_map, skip_list_index);
//     data_structure->add_edge(skip_list_index, skip_list_index);
//     data_structure->add_edge(skip_list_index, skip_list_node);
//     data_structure->add_edge(skip_list_node, skip_list_node);
//     data_structure->add_edge(skip_list_node, btree_partition);
//     data_structure->add_edge(btree_partition, btree_partition_data);
//     data_structure->add_edge(btree_partition_data, l_object);
//     data_structure->add_edge(l_object, btree_row);
//     data_structure->add_edge(btree_row, l_object);
//     data_structure->add_edge(l_object, buffer_cell);
//     data_structure->add_edge(buffer_cell, heap_byte_buffer);
//     data_structure->add_edge(heap_byte_buffer, l_byte);

//     _data_structure_types.add(data_structure);
    
// }


// void G1DataStructureManager::initialize_predefined_data_structures() {
//     // [[D -> [D: 96.34%
//     // smile/neighbor/KDTree$Node -> smile/neighbor/KDTree$Node: 3.62%
//     // start recursive
//     // [[D -> [D : weight_factor: 96.34% size_factor: 0.9633962297497565 weight: 3834460710, count: 7802855
//     // num: 0
//     // smile/neighbor/KDTree$Node -> smile/neighbor/KDTree$Node : weight_factor: 3.62% size_factor: 0.036179536402381314 weight: 143999952, count: 23999992
//     Symbol* ll_double = SymbolTable::new_symbol("[[D");
//     Symbol* l_double = SymbolTable::new_symbol("[D");
//     Symbol* kd_tree_node = SymbolTable::new_symbol("smile/neighbor/KDTree$Node");
//     Symbol* kd_tree = SymbolTable::new_symbol("smile/neighbor/KDTree");
//     G1DataStructure* data_structure = new G1DataStructure();
//     data_structure->add_root(ll_double);
//     data_structure->add_edge(ll_double, l_double);
//     data_structure->add_root(kd_tree);
//     data_structure->add_edge(kd_tree, kd_tree_node);
//     data_structure->add_edge(kd_tree_node, kd_tree_node);

//     _data_structure_types.add(data_structure);
// }


// void G1DataStructureManager::initialize_predefined_data_structures() {
//     // [Lorg/apache/spark/util/collection/CompactBuffer; -> org/apache/spark/util/collection/CompactBuffer: 28.80%
//     // [Lscala/Tuple2; -> scala/Tuple2$mcII$sp: 14.22%
//     // [Ljava/lang/Object; -> scala/Tuple2$mcII$sp: 14.20%
//     // [Ljava/lang/Object; -> scala/Tuple2: 11.45%
//     // [Ljava/lang/Object; -> [Lorg/apache/spark/util/collection/CompactBuffer;: 9.60%
//     // org/apache/spark/util/collection/CompactBuffer -> java/lang/Integer: 7.54%
//     // [Ljava/lang/Object; -> java/lang/Integer: 6.32%
//     // org/apache/spark/util/collection/CompactBuffer -> [Ljava/lang/Object;: 3.52%
//     // org/apache/spark/storage/memory/DeserializedMemoryEntry -> [Lscala/Tuple2;: 2.63%

//     // org/apache/spark/util/collection/PartitionedAppendOnlyMap -> [Ljava/lang/Object;: 0.20%, weight: 7209098, count: 69
//     // org/apache/spark/util/collection/SizeTrackingAppendOnlyMap -> [Ljava/lang/Object;: 0.07%, weight: 2449452, count: 22

//     Symbol* size_tracking_append_only_map = SymbolTable::new_symbol("org/apache/spark/util/collection/SizeTrackingAppendOnlyMap");
//     Symbol* partitioned_append_only_map = SymbolTable::new_symbol("org/apache/spark/util/collection/PartitionedAppendOnlyMap");
//     Symbol* compact_buffer = SymbolTable::new_symbol("org/apache/spark/util/collection/CompactBuffer");
//     Symbol* l_compact_buffer = SymbolTable::new_symbol("[Lorg/apache/spark/util/collection/CompactBuffer;");

//     Symbol* l_object = SymbolTable::new_symbol("[Ljava/lang/Object;");
//     Symbol* tuple2 = SymbolTable::new_symbol("scala/Tuple2");
//     Symbol* tuple2_mcII_sp = SymbolTable::new_symbol("scala/Tuple2$mcII$sp");
//     Symbol* l_tuple2_mcII_sp = SymbolTable::new_symbol("[Lscala/Tuple2;");
//     Symbol* deserialized_memory_entry = SymbolTable::new_symbol("org/apache/spark/storage/memory/DeserializedMemoryEntry");
//     Symbol* integer = SymbolTable::new_symbol("java/lang/Integer");
//     // Symbol* l_integer = SymbolTable::new_symbol("[I");
//     // Symbol* l_double = SymbolTable::new_symbol("[D");
//     // Symbol* double_object = SymbolTable::new_symbol("java/lang/Double");

//     G1DataStructure* data_structure = new G1DataStructure();
//     data_structure->add_root(deserialized_memory_entry);
//     data_structure->add_root(size_tracking_append_only_map);
//     // data_structure->add_root(l_tuple2_mcII_sp);
//     data_structure->add_root(partitioned_append_only_map);
//     // data_structure->add_root(l_object);
//     data_structure->add_edge(deserialized_memory_entry, l_tuple2_mcII_sp);
//     data_structure->add_edge(l_tuple2_mcII_sp, tuple2_mcII_sp);
//     data_structure->add_edge(size_tracking_append_only_map, l_object);
//     data_structure->add_edge(partitioned_append_only_map, l_object);
//     data_structure->add_edge(l_object, l_compact_buffer);
//     data_structure->add_edge(l_compact_buffer, compact_buffer);
//     // data_structure->add_edge(compact_buffer, double_object);
//     data_structure->add_edge(compact_buffer, l_object);
//     data_structure->add_edge(compact_buffer, integer);
//     // data_structure->add_edge(compact_buffer, tuple2);
//     data_structure->add_edge(tuple2, tuple2_mcII_sp);



//     data_structure->add_edge(l_object, tuple2_mcII_sp);
//     data_structure->add_edge(l_object, tuple2);
//     data_structure->add_edge(l_object, integer);
//     data_structure->add_edge(tuple2, integer);
//     // data_structure->add_edge(tuple2, double_object);
//     // data_structure->add_edge(tuple2, l_double);


//     _data_structure_types.add(data_structure);
// }


// void G1DataStructureManager::initialize_predefined_data_structures() {
//     // org/apache/spark/util/collection/SizeTrackingAppendOnlyMap

//     // [Lorg/apache/spark/util/collection/CompactBuffer; -> org/apache/spark/util/collection/CompactBuffer: 22.66%
//     // org/apache/spark/util/collection/CompactBuffer -> java/lang/Double: 11.03%
//     // [Ljava/lang/Object; -> scala/Tuple2$mcII$sp: 9.39%
//     // [Ljava/lang/Object; -> [Lorg/apache/spark/util/collection/CompactBuffer;: 7.55%
//     // [Ljava/lang/Object; -> scala/Tuple2: 7.46%
//     // org/apache/spark/util/collection/CompactBuffer -> [Ljava/lang/Object;: 6.54%
//     // scala/collection/mutable/ArrayBuilder$ofInt -> [I: 5.84%
//     // scala/Tuple2 -> java/lang/Integer: 3.78%
//     // [Ljava/lang/Object; -> java/lang/Integer: 3.53%
//     // scala/Tuple4 -> [I: 3.50%
//     // scala/Tuple2 -> [D: 3.41%
//     // net/jpountz/lz4/LZ4BlockOutputStream -> [B: 2.98%
//     // scala/collection/mutable/ArrayBuilder$ofFloat -> [F: 2.92%

//     // Symbol* size_tracking_append_only_map = SymbolTable::new_symbol("org/apache/spark/util/collection/SizeTrackingAppendOnlyMap");
//     // Symbol* compact_buffer = SymbolTable::new_symbol("org/apache/spark/util/collection/CompactBuffer");
//     // Symbol* l_compact_buffer = SymbolTable::new_symbol("[Lorg/apache/spark/util/collection/CompactBuffer;");

//     // Symbol* l_object = SymbolTable::new_symbol("[Ljava/lang/Object;");
//     // Symbol* tuple2_mcII_sp = SymbolTable::new_symbol("scala/Tuple2$mcII$sp");
//     // Symbol* tuple2 = SymbolTable::new_symbol("scala/Tuple2");
//     // Symbol* integer = SymbolTable::new_symbol("java/lang/Integer");
//     // Symbol* l_integer = SymbolTable::new_symbol("[I");
//     // Symbol* l_double = SymbolTable::new_symbol("[D");
//     // Symbol* double_object = SymbolTable::new_symbol("java/lang/Double");

//     // G1DataStructure* data_structure = new G1DataStructure();
//     // data_structure->add_root(size_tracking_append_only_map);
//     // data_structure->add_edge(size_tracking_append_only_map, l_object);
//     // data_structure->add_edge(l_object, l_compact_buffer);
//     // data_structure->add_edge(l_compact_buffer, compact_buffer);
//     // data_structure->add_edge(compact_buffer, double_object);
//     // data_structure->add_edge(compact_buffer, l_object);
//     // data_structure->add_edge(compact_buffer, integer);
//     // data_structure->add_edge(compact_buffer, tuple2);



//     // data_structure->add_edge(l_object, tuple2_mcII_sp);
//     // data_structure->add_edge(l_object, tuple2);
//     // data_structure->add_edge(l_object, integer);
//     // data_structure->add_edge(tuple2, integer);
//     // data_structure->add_edge(tuple2, double_object);
//     // data_structure->add_edge(tuple2, l_double);


//     // _data_structure_types.add(data_structure);
// }

// void G1DataStructureManager::initialize_predefined_data_structures() {
//     // com/quickserverlab/quickcached/DataCarrier -> [B: 78.52%, weight: 2952533103, count: 11330861
//     // java/lang/String -> [B: 2.98%, weight: 112179717, count: 22680195
//     // java/util/concurrent/locks/ReentrantReadWriteLock$ReadLock -> java/util/concurrent/locks/ReentrantReadWriteLock$NonfairSync: 2.71%, weight: 101975463, count: 11330607
//     // com/quickserverlab/quickcached/DataCarrier -> java/util/concurrent/locks/ReentrantReadWriteLock: 1.51%, weight: 56653110, count: 11330622
//     // com/quickserverlab/quickcached/DataCarrier -> java/lang/String: 1.21%, weight: 45323252, count: 11330813
//     // com/github/benmanes/caffeine/cache/PSWMS -> java/lang/String: 1.20%, weight: 45303956, count: 11325989
//     // com/github/benmanes/caffeine/cache/PSWMS -> com/quickserverlab/quickcached/cache/impl/caffeine/CacheValue : weight_factor: 100.00% size_factor: 0.7851643354077243 weight: 56652935, count: 11330587
//     // com/quickserverlab/quickcached/cache/impl/caffeine/CacheValue -> com/quickserverlab/quickcached/DataCarrier : weight_factor: 100.00% size_factor: 0.7851643354077243 weight: 79314284, count: 11330612
//     // java/util/concurrent/locks/ReentrantReadWriteLock$NonfairSync -> java/util/concurrent/locks/ReentrantReadWriteLock$Sync$ThreadLocalHoldCounter: 6.47%, weight: 22661266, count: 11330633
//     // java/util/concurrent/locks/ReentrantReadWriteLock$ReadLock -> java/util/concurrent/locks/ReentrantReadWriteLock$NonfairSync: 6.47%, weight: 101975463, count: 11330607

    
//     Symbol* byte_array = SymbolTable::new_symbol("[B");
//     Symbol* data_carrier = SymbolTable::new_symbol("com/quickserverlab/quickcached/DataCarrier");
//     Symbol* string = SymbolTable::new_symbol("java/lang/String");
//     Symbol* reentrant_read_write_lock = SymbolTable::new_symbol("java/util/concurrent/locks/ReentrantReadWriteLock");
//     Symbol* reentrant_read_write_lock_read_lock = SymbolTable::new_symbol("java/util/concurrent/locks/ReentrantReadWriteLock$ReadLock");
//     Symbol* reentrant_read_write_lock_write_lock = SymbolTable::new_symbol("java/util/concurrent/locks/ReentrantReadWriteLock$WriteLock");
//     Symbol* reentrant_read_write_lock_nonfair_sync = SymbolTable::new_symbol("java/util/concurrent/locks/ReentrantReadWriteLock$NonfairSync");
//     Symbol* reentrant_read_write_lock_sync_thread_local_hold_counter = SymbolTable::new_symbol("java/util/concurrent/locks/ReentrantReadWriteLock$Sync$ThreadLocalHoldCounter");
//     Symbol* concurrent_hash_map = SymbolTable::new_symbol("java/util/concurrent/ConcurrentHashMap");
//     Symbol* cache_value = SymbolTable::new_symbol("com/quickserverlab/quickcached/cache/impl/caffeine/CacheValue");
//     Symbol* pswms = SymbolTable::new_symbol("com/github/benmanes/caffeine/cache/PSWMS");
//     Symbol* time_instance = SymbolTable::new_symbol("java/time/Instant");

//     Symbol* concurrent_hash_map_node_array = SymbolTable::new_symbol("[Ljava/util/concurrent/ConcurrentHashMap$Node;");
//     Symbol* concurrent_hash_map_node = SymbolTable::new_symbol("java/util/concurrent/ConcurrentHashMap$Node");

//     G1DataStructure* data_structure = new G1DataStructure();
//     data_structure->add_root(concurrent_hash_map);

//     data_structure->add_edge(reentrant_read_write_lock, reentrant_read_write_lock_read_lock);
//     data_structure->add_edge(reentrant_read_write_lock, reentrant_read_write_lock_write_lock);
//     data_structure->add_edge(reentrant_read_write_lock_read_lock, reentrant_read_write_lock_nonfair_sync);
//     data_structure->add_edge(reentrant_read_write_lock_nonfair_sync, reentrant_read_write_lock_sync_thread_local_hold_counter);

//     data_structure->add_edge(string, byte_array);

//     data_structure->add_edge(data_carrier, byte_array);
//     data_structure->add_edge(data_carrier, string);
//     data_structure->add_edge(data_carrier, reentrant_read_write_lock);

//     data_structure->add_edge(cache_value, data_carrier);
//     data_structure->add_edge(cache_value, time_instance);
//     data_structure->add_edge(pswms, cache_value);
//     data_structure->add_edge(pswms, string);
//     data_structure->add_edge(pswms, pswms);

//     data_structure->add_edge(concurrent_hash_map, concurrent_hash_map_node_array);
//     data_structure->add_edge(concurrent_hash_map, concurrent_hash_map_node);
//     data_structure->add_edge(concurrent_hash_map_node_array, concurrent_hash_map_node);
//     data_structure->add_edge(concurrent_hash_map_node, concurrent_hash_map_node);
//     data_structure->add_edge(concurrent_hash_map_node, pswms);

//     _data_structure_types.add(data_structure);
    
// }


// void G1DataStructureManager::initialize_predefined_data_structures() {
//     // // java/nio/HeapByteBuffer -> [B : weight_factor: 99.05% size_factor: 0.9904943866047917 weight: 192886274471, count: 23383061
//     // // org/apache/hadoop/hbase/nio/SingleByteBuff -> java/nio/HeapByteBuffer : weight_factor: 99.95% size_factor: 0.989986457320577 weight: 187188064, count: 23398508
//     // // org/apache/hadoop/hbase/io/hfile/ExclusiveMemHFileBlock -> org/apache/hadoop/hbase/nio/SingleByteBuff : weight_factor: 99.59% size_factor: 0.9859389914678521 weight: 139936068, count: 23322678
//     // // org/apache/hadoop/hbase/io/hfile/LruCachedBlock -> org/apache/hadoop/hbase/io/hfile/ExclusiveMemHFileBlock : weight_factor: 100.00% size_factor: 0.9858907560111589 weight: 233210470, count: 23321047
//     // // java/util/concurrent/ConcurrentHashMap$Node -> org/apache/hadoop/hbase/io/hfile/LruCachedBlock : weight_factor: 98.88% size_factor: 0.9748735317195553 weight: 184479800, count: 23059975
//     // // [Ljava/util/concurrent/ConcurrentHashMap$Node; -> java/util/concurrent/ConcurrentHashMap$Node : weight_factor: 71.43% size_factor: 0.6963393706280268 weight: 83359275, count: 16671855
//     // // java/util/concurrent/ConcurrentHashMap$Node -> java/util/concurrent/ConcurrentHashMap$Node : weight_factor: 28.57% size_factor: 0.2785339940220882 weight: 33343500, count: 6668700
//     // // java/util/concurrent/ConcurrentHashMap -> [Ljava/util/concurrent/ConcurrentHashMap$Node; : weight_factor: 99.38% size_factor: 0.6919891642651064 weight: 587288, count: 4797
//     // Symbol* conc_hash_map_node_array = SymbolTable::new_symbol("[Ljava/util/concurrent/ConcurrentHashMap$Node;");
//     // Symbol* conc_hash_map_node = SymbolTable::new_symbol("java/util/concurrent/ConcurrentHashMap$Node");
//     // Symbol* lrucached_block = SymbolTable::new_symbol("org/apache/hadoop/hbase/io/hfile/LruCachedBlock");
//     // Symbol* exclusive_mem_hfile_block = SymbolTable::new_symbol("org/apache/hadoop/hbase/io/hfile/ExclusiveMemHFileBlock");
//     // Symbol* hf_ctx = SymbolTable::new_symbol("org/apache/hadoop/hbase/io/hfile/HFileContext");
//     // Symbol* single_byte_buff = SymbolTable::new_symbol("org/apache/hadoop/hbase/nio/SingleByteBuff");
//     // Symbol* heap_byte_buffer = SymbolTable::new_symbol("java/nio/HeapByteBuffer");
//     // Symbol* byte_array = SymbolTable::new_symbol("[B");

//     // // org/apache/hadoop/hbase/nio/RefCnt
//     // Symbol* ref_cnt = SymbolTable::new_symbol("org/apache/hadoop/hbase/nio/RefCnt");

//     // G1DataStructure* data_structure = new G1DataStructure();
//     // data_structure->add_root(conc_hash_map_node_array);
//     // data_structure->add_edge(conc_hash_map_node_array, conc_hash_map_node);
//     // data_structure->add_edge(conc_hash_map_node, conc_hash_map_node);
//     // data_structure->add_edge(conc_hash_map_node, lrucached_block);
//     // data_structure->add_edge(lrucached_block, exclusive_mem_hfile_block);
//     // data_structure->add_edge(exclusive_mem_hfile_block, single_byte_buff);
//     // data_structure->add_edge(exclusive_mem_hfile_block, hf_ctx);
//     // data_structure->add_edge(single_byte_buff, heap_byte_buffer);
//     // data_structure->add_edge(single_byte_buff, ref_cnt);
//     // data_structure->add_edge(heap_byte_buffer, byte_array);

//     // _data_structure_types.add(data_structure);
// }

// void G1DataStructureManager::initialize_predefined_data_structures() {
//     Symbol* s1 = SymbolTable::new_symbol("[Ledu/cmu/graphchi/ChiVertex;");
//     Symbol* ChiPointer = SymbolTable::new_symbol("edu/cmu/graphchi/datablocks/ChiPointer");
//     Symbol* s2 = SymbolTable::new_symbol("edu/cmu/graphchi/ChiVertex");
//     Symbol* s3 = SymbolTable::new_symbol("[I");

//     G1DataStructure* data_structure = new G1DataStructure();
//     data_structure->add_root(s1);
//     // data_structure->add_root(s2);

//     data_structure->add_edge(s1, s2);
//     data_structure->add_edge(s2, s3);
//     data_structure->add_edge(s2, ChiPointer);

//     // G1DataStructureRegionSet* data_structure_region_set = new G1DataStructureRegionSet(G1CollectedHeap::heap(), data_structure);
//     // _data_structures.add(data_structure_region_set);
//     _data_structure_types.add(data_structure);
// }

// void G1DataStructureManager::initialize_predefined_data_structures() {
//     Symbol* payload = SymbolTable::new_symbol("org/example/gctest/DataStructureTest$MyPayload");
//     Symbol* l_i = SymbolTable::new_symbol("[I");
//     Symbol* l_payload = SymbolTable::new_symbol("[Lorg/example/gctest/DataStructureTest$MyPayload;");
//     Symbol* my_ds = SymbolTable::new_symbol("org/example/gctest/DataStructureTest$MyDS");

//     G1DataStructure* data_structure = new G1DataStructure();
//     data_structure->add_root(my_ds);

//     data_structure->add_edge(my_ds, l_payload);
//     data_structure->add_edge(l_payload, payload);
//     data_structure->add_edge(payload, l_i);

//     _data_structure_types.add(data_structure);
// }



// void G1DataStructureManager::initialize_predefined_data_structures() {

//     // // [Lorg/apache/spark/graphx/Edge; -> org/apache/spark/graphx/Edge$mcI$sp

//     // Symbol* l_edge = SymbolTable::new_symbol("[Lorg/apache/spark/graphx/Edge;");
//     // Symbol* edge = SymbolTable::new_symbol("org/apache/spark/graphx/Edge$mcI$sp");

//     // G1DataStructure* data_structure = new G1DataStructure();
//     // data_structure->add_root(l_edge);
//     // data_structure->add_edge(l_edge, edge);

//     // _data_structure_types.add(data_structure);
    
// }

void G1DataStructureManager::initialize_predefined_data_structures() {
    Symbol* tree_node = SymbolTable::new_symbol("smile/clustering/BBDTree$Node");
    Symbol* l_d = SymbolTable::new_symbol("[D");
    G1DataStructure* data_structure = new G1DataStructure();
    data_structure->add_root(tree_node);
    data_structure->add_edge(tree_node, tree_node);
    data_structure->add_edge(tree_node, l_d);

    _data_structure_types.add(data_structure);
}

// void G1DataStructureManager::initialize_predefined_data_structures() {
//     Symbol* l_tuple3 = SymbolTable::new_symbol("[Lscala/Tuple3;");
//     Symbol* tuple3 = SymbolTable::new_symbol("scala/Tuple3");
//     Symbol* l_d = SymbolTable::new_symbol("[D");
//     Symbol* l_i = SymbolTable::new_symbol("[I");
//     Symbol* d = SymbolTable::new_symbol("java/lang/Double");

//     G1DataStructure* data_structure = new G1DataStructure();
//     data_structure->add_root(l_tuple3);
//     data_structure->add_edge(l_tuple3, tuple3);
//     data_structure->add_edge(tuple3, l_d);
//     data_structure->add_edge(tuple3, l_i);
//     data_structure->add_edge(tuple3, d);

//     _data_structure_types.add(data_structure);
    
// }

// void G1DataStructureManager::initialize_predefined_data_structures() {

//     // 'edu/stanford/nlp/parser/lexparser/ExhaustivePCFGParser',
//     // 'edu/stanford/nlp/parser/lexparser/LexicalizedParserQuery',
//     // '[[I',
//     // '[I',
//     // '[[[F',
//     // '[[F',
//     // '[F',
//     // 'edu/stanford/nlp/ie/crf/CRFClassifier',

//     Symbol* ExhaustivePCFGParser = SymbolTable::new_symbol("edu/stanford/nlp/parser/lexparser/ExhaustivePCFGParser");
//     Symbol* LexicalizedParserQuery = SymbolTable::new_symbol("edu/stanford/nlp/parser/lexparser/LexicalizedParserQuery");
//     Symbol* CRFClassifier = SymbolTable::new_symbol("edu/stanford/nlp/ie/crf/CRFClassifier");
//     Symbol* ll_i = SymbolTable::new_symbol("[[I");
//     Symbol* ll_f = SymbolTable::new_symbol("[[F");
//     Symbol* lll_f = SymbolTable::new_symbol("[[[F");
//     Symbol* l_f = SymbolTable::new_symbol("[F");
//     Symbol* l_i = SymbolTable::new_symbol("[I");

//     G1DataStructure* data_structure = new G1DataStructure();
//     // data_structure->add_root(LexicalizedParserQuery);
//     // data_structure->add_root(CRFClassifier);
//     data_structure->add_root(lll_f);

//     // data_structure->add_edge(LexicalizedParserQuery, ExhaustivePCFGParser);
//     // data_structure->add_edge(ExhaustivePCFGParser, ll_i);
//     // data_structure->add_edge(ExhaustivePCFGParser, lll_f);
//     // data_structure->add_edge(ll_i, l_i);
//     data_structure->add_edge(lll_f, ll_f);
//     data_structure->add_edge(ll_f, l_f);
//     data_structure->add_edge(CRFClassifier, ll_f);

//     // G1DataStructureRegionSet* data_structure_region_set = new G1DataStructureRegionSet(G1CollectedHeap::heap(), data_structure);
//     // _data_structures.add(data_structure_region_set);
//     _data_structure_types.add(data_structure);
// }



DataPLABMap* G1DataStructureManager::create_and_initialize_plab_map(uint num_alloc_buffers, size_t desired_plab_size, size_t tolerated_refills){
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    DataPLABMap* plab_map = new DataPLABMap(10);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        G1PLABAllocator::PLABData* plab_data = new G1PLABAllocator::PLABData();
        plab_data->initialize(num_alloc_buffers, desired_plab_size, tolerated_refills);
        plab_map->insert(data_structure, plab_data);
        p = p->next();
    }
    return plab_map;

    // return nullptr;
}

void G1DataStructureManager::delete_plab_map(DataPLABMap* plab_map) {
    DeleteClosure cl;
    plab_map->forEachClosure(&cl);
    delete plab_map;
}

void G1DataStructureManager::initialize_at_conc_start(){
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    _allocator = nullptr;
    _evacuation_info = nullptr;
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        // log_info(gc)("set data structure not alive %u", data_structure->id());
        data_structure->set_alive(false);
        p = p->next();
    }
}

void G1DataStructureManager::data_structures_instances_iterate(G1DataStructureRegionSetClosure* cl){
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        cl->do_data_structure_instance(data_structure);
        p = p->next();
    }
}

void G1DataStructureManager::clear_all_out_cards(){
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        data_structure->clear_out_cards();
        p = p->next();
    }
}

void G1DataStructureManager::clear_all_instances() {
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        log_info(gc)("delete instance");
        G1DataStructureRegionSet* data_structure = *p->data();
        delete data_structure;
        p = p->next();
    }
    _data_structures.clear();
    _present_id = 0;
}

void G1DataStructureManager::remove_instance(G1DataStructureRegionSet* data_structure) {
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    if (_data_structures.remove(data_structure)) {
        log_info(gc)("remove data structure %u", data_structure->id());
        delete data_structure;
    } else {
        log_info(gc)("data structure %u not found", data_structure->id());
    }
}

void G1DataStructureManager::remove_dead_instances() {
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        if (!data_structure->is_alive()) {
            log_info(gc)("remove dead data structure %u %p", data_structure->id(), data_structure);
            p = p->next();
            _data_structures.remove(data_structure);
            delete data_structure;
        } else {
            p = p->next();
        }
    }
}

void G1DataStructureManager::verify_all(){
    MutexLocker ml(&_data_structures_lock, Mutex::_no_safepoint_check_flag);
    LinkedListNode<G1DataStructureRegionSet*>* p = _data_structures.head();
    while (p != nullptr) {
        G1DataStructureRegionSet* data_structure = *p->data();
        data_structure->verify();
        p = p->next();
    }
}