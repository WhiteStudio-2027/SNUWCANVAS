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
}
