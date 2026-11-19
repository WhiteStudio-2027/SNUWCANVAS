#pragma once
#include "common.hpp"
namespace snuw {
inline constexpr size_t thumbnailCapacity=256;
struct GalleryLayout {
 int columns=3;float tileWidth=172,tileHeight=204,gap=10;
 explicit GalleryLayout(float width,float preferred=180){columns=std::clamp(int((width+gap)/preferred),1,8);tileWidth=std::max(1.f,(width-gap*(columns-1))/columns);tileHeight=tileWidth*1.12f+gap;}
 int rows(size_t count)const{return int((count+columns-1)/columns);}
 float contentHeight(size_t count)const{return rows(count)*tileHeight;}
 Rect tile(Rect area,int index,float scroll)const{return {area.x+(index%columns)*(tileWidth+gap),area.y+(index/columns)*tileHeight-scroll,tileWidth,tileHeight-gap};}
 int indexAt(Rect area,float x,float y,float scroll,size_t count)const{if(!area.hit(x,y))return -1;int col=int((x-area.x)/(tileWidth+gap)),row=int((y-area.y+scroll)/tileHeight);if(col>=columns||(x-area.x)-col*(tileWidth+gap)>=tileWidth||(y-area.y+scroll)-row*tileHeight>=tileHeight-gap)return -1;int index=row*columns+col;return index<int(count)?index:-1;}
};
struct GalleryChannelGroup {std::string id;std::wstring name;size_t first=0,count=0;};
struct GalleryViewLayout {
 GalleryLayout grid;const std::vector<GalleryChannelGroup>& groups;bool grouped;std::vector<float> tops;float total=0;static constexpr float headerHeight=32;
 GalleryViewLayout(float width,float preferred,const std::vector<GalleryChannelGroup>& value,bool enabled):grid(width,preferred),groups(value),grouped(enabled&&!value.empty()){if(grouped)for(auto& group:groups){tops.push_back(total);total+=headerHeight+grid.contentHeight(group.count)+8;}}
 float contentHeight(size_t count)const{return grouped?total:grid.contentHeight(count);}
 size_t groupAt(size_t index)const{auto it=std::upper_bound(groups.begin(),groups.end(),index,[](size_t i,const GalleryChannelGroup& g){return i<g.first;});return it==groups.begin()?0:size_t(it-groups.begin()-1);}
 Rect tile(Rect area,int index,float scroll)const{if(!grouped)return grid.tile(area,index,scroll);auto i=groupAt(size_t(index));area.y+=tops[i]+headerHeight;return grid.tile(area,index-int(groups[i].first),scroll);}
 Rect header(Rect area,size_t index,float scroll)const{return {area.x,area.y+tops[index]-scroll,area.w,headerHeight};}
 int indexAt(Rect area,float x,float y,float scroll,size_t count)const{
  if(!grouped)return grid.indexAt(area,x,y,scroll,count);if(!area.hit(x,y))return -1;float local=y-area.y+scroll;
  auto it=std::upper_bound(tops.begin(),tops.end(),local);if(it==tops.begin())return -1;size_t i=size_t(it-tops.begin()-1);if(local<tops[i]+headerHeight)return -1;Rect body{area.x,area.y+tops[i]+headerHeight-scroll,area.w,grid.contentHeight(groups[i].count)};int found=grid.indexAt(body,x,y,0,groups[i].count);return found<0?-1:int(groups[i].first)+found;
 }
 std::pair<int,int> visibleRange(Rect area,float scroll,size_t count)const{
  if(!grouped){int first=std::max(0,int(scroll/grid.tileHeight)*grid.columns);return {first,std::min(int(count),first+(int(area.h/grid.tileHeight)+2)*grid.columns)};}
  int first=int(count),last=0;for(size_t i=0;i<groups.size();i++){float begin=tops[i]+headerHeight,end=begin+grid.contentHeight(groups[i].count);if(end<scroll||begin>scroll+area.h)continue;int row=std::max(0,int((scroll-begin)/grid.tileHeight)),finish=std::max(0,int((scroll+area.h-begin)/grid.tileHeight)+2);first=std::min(first,int(groups[i].first)+row*grid.columns);last=std::max(last,int(groups[i].first)+std::min(int(groups[i].count),finish*grid.columns));}return first<last?std::pair<int,int>{first,last}:std::pair<int,int>{0,0};
 }
};
}
