#pragma once
#include "common.hpp"
namespace snuw {
// Evict by last use, never by filename: older images may sort before every cached image.
template<class T> class ThumbnailCache {
 struct Entry { T value; uint64_t used; };std::map<fs::path,Entry> entries;uint64_t clock=0;size_t capacity;
public:
 explicit ThumbnailCache(size_t limit=64):capacity(std::max(size_t(1),limit)){}
 T* find(const fs::path& path){auto it=entries.find(path);if(it==entries.end())return nullptr;it->second.used=++clock;// ComPtr overloads &: using it here releases the cached image.
 return std::addressof(it->second.value);}
 void insert(const fs::path& path,T value){entries.insert_or_assign(path,Entry{std::move(value),++clock});while(entries.size()>capacity){auto oldest=std::min_element(entries.begin(),entries.end(),[](auto& a,auto& b){return a.second.used<b.second.used;});entries.erase(oldest);}}
 void clear(){entries.clear();clock=0;}size_t size()const{return entries.size();}
};
}
