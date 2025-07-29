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
        } else if(from_region->is_humongous()){
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

void G1DataStructureManager::initialize_predefined_data_structures() {
    // // java/nio/HeapByteBuffer -> [B : weight_factor: 99.05% size_factor: 0.9904943866047917 weight: 192886274471, count: 23383061
    // // org/apache/hadoop/hbase/nio/SingleByteBuff -> java/nio/HeapByteBuffer : weight_factor: 99.95% size_factor: 0.989986457320577 weight: 187188064, count: 23398508
    // // org/apache/hadoop/hbase/io/hfile/ExclusiveMemHFileBlock -> org/apache/hadoop/hbase/nio/SingleByteBuff : weight_factor: 99.59% size_factor: 0.9859389914678521 weight: 139936068, count: 23322678
    // // org/apache/hadoop/hbase/io/hfile/LruCachedBlock -> org/apache/hadoop/hbase/io/hfile/ExclusiveMemHFileBlock : weight_factor: 100.00% size_factor: 0.9858907560111589 weight: 233210470, count: 23321047
    // // java/util/concurrent/ConcurrentHashMap$Node -> org/apache/hadoop/hbase/io/hfile/LruCachedBlock : weight_factor: 98.88% size_factor: 0.9748735317195553 weight: 184479800, count: 23059975
    // // [Ljava/util/concurrent/ConcurrentHashMap$Node; -> java/util/concurrent/ConcurrentHashMap$Node : weight_factor: 71.43% size_factor: 0.6963393706280268 weight: 83359275, count: 16671855
    // // java/util/concurrent/ConcurrentHashMap$Node -> java/util/concurrent/ConcurrentHashMap$Node : weight_factor: 28.57% size_factor: 0.2785339940220882 weight: 33343500, count: 6668700
    // // java/util/concurrent/ConcurrentHashMap -> [Ljava/util/concurrent/ConcurrentHashMap$Node; : weight_factor: 99.38% size_factor: 0.6919891642651064 weight: 587288, count: 4797
    // Symbol* conc_hash_map_node_array = SymbolTable::new_symbol("[Ljava/util/concurrent/ConcurrentHashMap$Node;");
    // Symbol* conc_hash_map_node = SymbolTable::new_symbol("java/util/concurrent/ConcurrentHashMap$Node");
    // Symbol* lrucached_block = SymbolTable::new_symbol("org/apache/hadoop/hbase/io/hfile/LruCachedBlock");
    // Symbol* exclusive_mem_hfile_block = SymbolTable::new_symbol("org/apache/hadoop/hbase/io/hfile/ExclusiveMemHFileBlock");
    // Symbol* hf_ctx = SymbolTable::new_symbol("org/apache/hadoop/hbase/io/hfile/HFileContext");
    // Symbol* single_byte_buff = SymbolTable::new_symbol("org/apache/hadoop/hbase/nio/SingleByteBuff");
    // Symbol* heap_byte_buffer = SymbolTable::new_symbol("java/nio/HeapByteBuffer");
    // Symbol* byte_array = SymbolTable::new_symbol("[B");

    // // org/apache/hadoop/hbase/nio/RefCnt
    // Symbol* ref_cnt = SymbolTable::new_symbol("org/apache/hadoop/hbase/nio/RefCnt");

    // G1DataStructure* data_structure = new G1DataStructure();
    // data_structure->add_root(conc_hash_map_node_array);
    // data_structure->add_edge(conc_hash_map_node_array, conc_hash_map_node);
    // data_structure->add_edge(conc_hash_map_node, conc_hash_map_node);
    // data_structure->add_edge(conc_hash_map_node, lrucached_block);
    // data_structure->add_edge(lrucached_block, exclusive_mem_hfile_block);
    // data_structure->add_edge(exclusive_mem_hfile_block, single_byte_buff);
    // data_structure->add_edge(exclusive_mem_hfile_block, hf_ctx);
    // data_structure->add_edge(single_byte_buff, heap_byte_buffer);
    // data_structure->add_edge(single_byte_buff, ref_cnt);
    // data_structure->add_edge(heap_byte_buffer, byte_array);

    // _data_structure_types.add(data_structure);
}

// void G1DataStructureManager::initialize_predefined_data_structures() {
//     // Symbol* s1 = SymbolTable::new_symbol("[Ledu/cmu/graphchi/ChiVertex;");
//     // Symbol* ChiPointer = SymbolTable::new_symbol("edu/cmu/graphchi/datablocks/ChiPointer");
//     // Symbol* s2 = SymbolTable::new_symbol("edu/cmu/graphchi/ChiVertex");
//     // Symbol* s3 = SymbolTable::new_symbol("[I");

//     // G1DataStructure* data_structure = new G1DataStructure();
//     // data_structure->add_root(s1);
//     // // data_structure->add_root(s2);

//     // data_structure->add_edge(s1, s2);
//     // data_structure->add_edge(s2, s3);
//     // data_structure->add_edge(s2, ChiPointer);

//     // // G1DataStructureRegionSet* data_structure_region_set = new G1DataStructureRegionSet(G1CollectedHeap::heap(), data_structure);
//     // // _data_structures.add(data_structure_region_set);
//     // _data_structure_types.add(data_structure);
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

// void G1DataStructureManager::initialize_predefined_data_structures() {
//     Symbol* tree_node = SymbolTable::new_symbol("smile/clustering/BBDTree$Node");
//     Symbol* l_d = SymbolTable::new_symbol("[D");
//     G1DataStructure* data_structure = new G1DataStructure();
//     data_structure->add_root(tree_node);
//     data_structure->add_edge(tree_node, tree_node);
//     data_structure->add_edge(tree_node, l_d);

//     _data_structure_types.add(data_structure);
// }

// void G1DataStructureManager::initialize_predefined_data_structures() {
//     // Symbol* l_tuple3 = SymbolTable::new_symbol("[Lscala/Tuple3;");
//     // Symbol* tuple3 = SymbolTable::new_symbol("scala/Tuple3");
//     // Symbol* l_d = SymbolTable::new_symbol("[D");
//     // Symbol* l_i = SymbolTable::new_symbol("[I");
//     // Symbol* d = SymbolTable::new_symbol("java/lang/Double");

//     // G1DataStructure* data_structure = new G1DataStructure();
//     // data_structure->add_root(l_tuple3);
//     // data_structure->add_edge(l_tuple3, tuple3);
//     // data_structure->add_edge(tuple3, l_d);
//     // data_structure->add_edge(tuple3, l_i);
//     // data_structure->add_edge(tuple3, d);

//     // _data_structure_types.add(data_structure);
    
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