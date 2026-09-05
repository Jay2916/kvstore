#include "../include/HashTableStorage.hpp"
#include "../include/hashtable.hpp"


#define container_of(ptr, T, member) \
    ((T *)( (char *)ptr - offsetof(T, member) ))


bool entry_eq(HNode *hnode1, HNode *hnode2){
    Entry *e1 = container_of(hnode1, Entry, node);
    Entry *e2 = container_of(hnode2, Entry, node);
    return e1->key == e2->key;
}

std::optional<std::vector<std::byte>>
HashTableStorage::get(std::span<const std::byte> key) {
    Entry lookup{};
    lookup.key.assign(key.begin(), key.end()); //this copying the key to lookup.key, which takes time
    lookup.node.hcode = hash_bytes(key);
    HNode *node = hm_lookup(&(this->hmap), &lookup.node, entry_eq);
    if(node){
        Entry *e = container_of(node, Entry, node);
        return e->value;
    }
    else{
        return std::nullopt;
    }
}

//set cant fail since it adds key if not already there
void HashTableStorage::set(std::span<const std::byte> key, std::span<const std::byte> value) {
    Entry lookup{};
    lookup.key.assign(key.begin(), key.end());
    lookup.node.hcode = hash_bytes(key);
    HNode *node = hm_lookup(&(this->hmap), &lookup.node, entry_eq);
    if(node){
        container_of(node, Entry, node)->value.assign(value.begin(), value.end());
    }
    else{
        Entry *e = new Entry();
        e->node.hcode = lookup.node.hcode;
        e->node.next = NULL;
        e->key.assign(key.begin(), key.end());
        e->value.assign(value.begin(), value.end());
        hm_insert(&(this->hmap), &e->node);
    }
}
bool HashTableStorage::del(std::span<const std::byte> key) {
    Entry lookup{};
    lookup.key.assign(key.begin(), key.end());
    lookup.node.hcode = hash_bytes(key);
    HNode *node = hm_delete(&(this->hmap), &lookup.node, entry_eq);
    if(node){
        Entry *e = container_of(node, Entry, node);
        delete e;
    }
    else{
        return false;
    }
    return true;
}
void HashTableStorage::clear(){
    hm_clear(&(this->hmap));
}
size_t HashTableStorage::size(){
    return hm_size(&(this->hmap));
}

uint64_t HashTableStorage::hash_bytes(std::span<const std::byte> data){
    uint64_t hash = 14695981039346656037ULL;
    for (std::byte b : data) {
        hash ^= std::to_integer<uint8_t>(b);
        hash *= 1099511628211ULL;
    }
    return hash; 
}