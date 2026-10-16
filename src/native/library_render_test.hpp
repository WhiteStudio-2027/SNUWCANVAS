#pragma once
#include "app.hpp"
#include <d3d11.h>
namespace snuw {
inline int libraryRenderTest(const fs::path& folder){try{
 auto store=std::make_shared<Store>(folder/"state");store->load();App app(store);app.width=1400;app.height=1000;app.leftWidth=205;app.rightWidth=460;
 for(int i=359;i>=0;i--)app.paths.push_back(folder/(L"image-"+std::to_wstring(i)+L".png"));
 Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context),"Library GPU creation failed");
 D3D11_TEXTURE2D_DESC desc{};desc.Width=2800;desc.Height=2000;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;Com<ID3D11Texture2D> texture;check(device->CreateTexture2D(&desc,nullptr,&texture),"Library texture creation failed");Com<IDXGISurface> surface;check(texture.As(&surface),"Library surface failed");
 check(app.renderer.factory->CreateDxgiSurfaceRenderTarget(surface.Get(),D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_HARDWARE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED)),&app.renderer.target),"Library render target failed");app.renderer.target->CreateSolidColorBrush(color(0xffffff),&app.renderer.brush);
 desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.BindFlags=0;Com<ID3D11Texture2D> staging;check(device->CreateTexture2D(&desc,nullptr,&staging),"Library readback failed");int pages=0;
 for(float libraryWidth:std::vector<float>{410.f,650.f,900.f}){app.rightWidth=libraryWidth;for(float dpi:std::vector<float>{1.f,1.5f,2.f}){app.dpi=dpi;for(int page:std::vector<int>{0,10,20,30,40,50,60,70,80,90,100,110,116,0,60}){
  app.scroll=page*138.f;bool loaded=false;
  for(int frame=0;frame<800&&!loaded;frame++){app.paint();app.tick();loaded=true;GalleryLayout grid(app.browser.w);int first=int(app.scroll/grid.tileHeight)*grid.columns,last=std::min(int(app.paths.size()),first+(int(app.browser.h/grid.tileHeight)+1)*grid.columns);for(int i=first;i<last;i++)if(!app.thumbs.find(app.paths[i]))loaded=false;if(!loaded)Sleep(5);}
  if(!loaded)throw std::runtime_error("Actual gallery pipeline left thumbnails pending");app.paint();context->CopyResource(staging.Get(),texture.Get());D3D11_MAPPED_SUBRESOURCE mapped;check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped),"Library pixel readback failed");int drawn=0,expected=0;GalleryLayout grid(app.browser.w);int first=int(app.scroll/grid.tileHeight)*grid.columns;
  for(int row=0;row<3;row++)for(int column=0;column<grid.columns;column++){auto rect=grid.tile(app.browser,first+row*grid.columns+column,app.scroll);if(rect.y<app.browser.y||rect.y+rect.h>app.browser.y+app.browser.h)continue;expected++;int x=int((rect.x+rect.w/2)*dpi),y=int((rect.y+rect.h/2)*dpi);auto pixel=(const BYTE*)mapped.pData+y*mapped.RowPitch+x*4;if(pixel[0]==0x99&&pixel[1]==0x66&&pixel[2]==0x33)drawn++;}
  context->Unmap(staging.Get(),0);if(!expected||drawn!=expected)throw std::runtime_error("Actual responsive gallery has blank GPU pixels: "+std::to_string(drawn)+"/"+std::to_string(expected));pages++;
 }
 }
 }
 app.engine->stopping=true;app.engine->stop();atomicWrite(folder/"native-library-render-result.json",json{{"ok",true},{"gpu_pixel_pages",pages},{"actual_app_paint",true},{"dpi_scales",3},{"library_widths",3},{"gui_opened",false}}.dump(2));return 0;
 }catch(const std::exception& e){atomicWrite(folder/"native-library-render-result.json",json{{"ok",false},{"error",e.what()},{"gui_opened",false}}.dump(2));return 1;}}
}
