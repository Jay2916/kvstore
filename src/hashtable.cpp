#include <stdlib.h>
#include <assert.h>
#include "../include/hashtable.h"

const size_t k_max_load_factor = 2;
const size_t k_init_size = 4;
const size_t k_rehash_work = 64;



static void h_init(HTab *htab, size_t n){
    assert(n > 0 && ((n & (n-1)) == 0));
    htab->tab = (HNode**)calloc(n, sizeof(HNode*));
    htab->size = 0;
    htab->mask = n-1;
}
static void h_insert(HTab *htab, HNode *hnode){
    size_t pos = hnode->hcode & htab->mask;
    hnode->next = htab->tab[pos];
    htab->tab[pos] = hnode;
    htab->size++;
}
static HNode** h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *)){
    if(htab->tab == NULL){
        return NULL;
    }
    size_t pos = key->hcode & htab->mask;
    HNode **from = &htab->tab[pos];
    for(HNode *cur; (cur = *from) != NULL; from = &(cur->next)){
        if(cur->hcode == key->hcode && eq(cur, key)){
            return from;
        }
    }
    return NULL;
}
static HNode* h_detach(HTab *htab, HNode **from){
    HNode *node = *from; //assumes from !=NULL or *from != NULL
    *from = node->next;
    htab->size--;
    return node;
}

static void hm_trigger_rehash(HMap *hmap){
    hmap->older = hmap->newer;
    h_init(&hmap->newer, (hmap->older.mask + 1) * 2);
    hmap->migrate_pos = 0;
}
static void hm_help_rehash(HMap *hmap){
    size_t work = 0;
    while(hmap->older.size > 0 && work < k_rehash_work){
        size_t pos = hmap->migrate_pos;
        HNode **from = &hmap->older.tab[pos];
        if(*from == NULL){
            hmap->migrate_pos++;
            continue;
        }
        HNode *hnode = h_detach(&hmap->older, from);
        h_insert(&hmap->newer, hnode);
        work++;
    }
    if(hmap->older.size == 0 && hmap->older.tab){
        free(hmap->older.tab);
        hmap->older = HTab{};
    }
}

void hm_insert(HMap *hmap, HNode *hnode){
    if(!hmap->newer.tab){
        h_init(&hmap->newer, k_init_size);
    }
    h_insert(&hmap->newer, hnode);
    if(!hmap->older.tab){
        if(hmap->newer.size > (hmap->newer.mask + 1) * k_max_load_factor){
            hm_trigger_rehash(hmap);
        }
    }
    hm_help_rehash(hmap);

}
HNode* hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)){
    hm_help_rehash(hmap);
    HNode **from;
    if((from = h_lookup(&hmap->newer, key, eq))){
        return h_detach(&hmap->newer, from);
    }
    if((from = h_lookup(&hmap->older, key, eq))){
        return h_detach(&hmap->older, from);
    }
    return NULL;
}
HNode* hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)){
    hm_help_rehash(hmap);
    HNode **from = h_lookup(&hmap->newer, key, eq);
    if(!from){
        from = h_lookup(&hmap->older, key, eq);
    }
    return from? *from : NULL;
}
void hm_clear(HMap *hmap){
    if(hmap->older.tab){
        free(hmap->older.tab);
    }
    if(hmap->newer.tab){
        free(hmap->newer.tab);
    }
    hmap->older = HTab{};
    hmap->newer = HTab{};
    hmap->migrate_pos = 0;
}
size_t hm_size(HMap *hmap){
    return hmap->older.size + hmap->newer.size;
}