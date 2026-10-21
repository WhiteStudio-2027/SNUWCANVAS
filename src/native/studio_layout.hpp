#pragma once
#include "common.hpp"
namespace snuw {
// A compact editor next to a flexible image canvas. Sizes are logical pixels.
struct StudioLayout {
 float sidebar,editor,canvas;
 StudioLayout(float width,int page,float sidebarWidth=184):sidebar(sidebarWidth){
  float preferred=page==0?380.f:520.f;
  editor=std::max(300.f,std::min(preferred,width-sidebar-320));
  canvas=std::max(280.f,width-sidebar-editor-30);
 }
 static float positiveHeight(float height){return std::clamp((height-500)*.4f,64.f,160.f);}
 static float negativeHeight(float height){return std::clamp((height-500)*.2f,40.f,80.f);}
};
}
