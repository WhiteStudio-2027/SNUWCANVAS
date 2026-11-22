#pragma once
namespace snuw {
// 프로필 화면을 이미지 파일로 그려 레이아웃을 눈으로 확인하기 위한 도구 (--account-preview <폴더>)
inline int accountPreview(const fs::path& folder){try{
 LoadLibraryW(L"Msftedit.dll");INITCOMMONCONTROLSEX controls{sizeof controls,ICC_STANDARD_CLASSES};InitCommonControlsEx(&controls);fs::create_directories(folder);
 auto store=std::make_shared<Store>(folder/"state");store->load();store->useWorkspace(std::string(64,'a'));App app(store);
 auto parent=CreateWindowExW(0,L"STATIC",L"",0,0,0,1000,900,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);app.hwnd=parent;app.page=13;app.dpi=1;app.hardcore();
 app.accountProfile={{"account_id","owner"},{"nickname","admin"},{"email","user@example.com"},{"role","develop"},{"bio","admin"},{"profile_version",std::string(64,'0')},{"profile_media_supported",true}};
 for(int scene=0;scene<3;scene++){
  float W=scene==2?360.f:720.f,H=scene==2?760.f:860.f;app.width=W+40;app.height=H;bool hard=false;app.renderer.hardcore=hard;
  Com<IWICBitmap> bitmap;check(app.renderer.wic->CreateBitmap(UINT(W+40),UINT(H),GUID_WICPixelFormat32bppPBGRA,WICBitmapCacheOnLoad,&bitmap),"bitmap");
  Com<ID2D1RenderTarget> target;check(app.renderer.factory->CreateWicBitmapRenderTarget(bitmap.Get(),D2D1::RenderTargetProperties(),&target),"wic target");app.renderer.target=target;app.renderer.brush.Reset();app.renderer.target->CreateSolidColorBrush(color(0xffffff),&app.renderer.brush);app.renderer.formats.clear();
  app.accountMediaBitmaps[0].Reset();app.accountMediaBitmaps[1].Reset();
  if(scene!=1){std::vector<BYTE> banner(1280*320*4),avatar(384*384*4);for(int y=0;y<320;y++)for(int x=0;x<1280;x++){auto* p=&banner[(size_t(y)*1280+x)*4];p[0]=BYTE(180-y/4);p[1]=BYTE(110+x/20);p[2]=BYTE(90+y/3);p[3]=255;}for(int y=0;y<384;y++)for(int x=0;x<384;x++){auto* p=&avatar[(size_t(y)*384+x)*4];p[0]=BYTE(90+x/4);p[1]=BYTE(160);p[2]=BYTE(220-y/4);p[3]=255;}app.accountMediaBitmaps[1]=app.renderer.pixels(1280,320,banner);app.accountMediaBitmaps[0]=app.renderer.pixels(384,384,avatar);}
  app.accountMediaMessage=scene==1?L"":L"";Rect c{20,14,W,H-28};app.buttons.clear();
  app.renderer.target->BeginDraw();app.renderer.target->Clear(color(0x1b1b1c));app.label(L"내 계정",{60,26,300,36},true,0xf2f2f4,18);app.accountPage(c);check(app.renderer.target->EndDraw(),"draw");
  Com<IWICStream> stream;app.renderer.wic->CreateStream(&stream);check(stream->InitializeFromFilename((folder/("account-preview-"+std::to_string(scene)+".png")).wstring().c_str(),GENERIC_WRITE),"stream");
  Com<IWICBitmapEncoder> encoder;app.renderer.wic->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder);encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache);Com<IWICBitmapFrameEncode> frame;Com<IPropertyBag2> props;encoder->CreateNewFrame(&frame,&props);frame->Initialize(props.Get());frame->SetSize(UINT(W+40),UINT(H));WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;frame->SetPixelFormat(&format);frame->WriteSource(bitmap.Get(),nullptr);frame->Commit();encoder->Commit();
 }
 return 0;}catch(const std::exception& e){atomicWrite(folder/"account-preview-error.txt",e.what());return 1;}}
// 아주 큰 사진을 압축 경로로 줄일 수 있는지 확인한다 (--account-photo-test <폴더>)
inline int accountPhotoTest(const fs::path& folder){try{
 LoadLibraryW(L"Msftedit.dll");fs::create_directories(folder);auto store=std::make_shared<Store>(folder/"state");store->load();store->useWorkspace(std::string(64,'a'));App app(store);
 const UINT W=5000,H=3500;std::vector<BYTE> pixels(size_t(W)*H*4);uint32_t seed=12345;for(size_t i=0;i<pixels.size();i+=4){seed=seed*1664525u+1013904223u;pixels[i]=BYTE(seed>>8);pixels[i+1]=BYTE(seed>>16);pixels[i+2]=BYTE(seed>>24);pixels[i+3]=255;}
 Com<IWICImagingFactory> wic;check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&wic)),"wic");Com<IWICBitmap> bitmap;check(wic->CreateBitmapFromMemory(W,H,GUID_WICPixelFormat32bppBGRA,W*4,UINT(pixels.size()),pixels.data(),&bitmap),"bitmap");
 auto big=folder/"big.png";{Com<IWICStream> stream;wic->CreateStream(&stream);check(stream->InitializeFromFilename(big.wstring().c_str(),GENERIC_WRITE),"stream");Com<IWICBitmapEncoder> encoder;wic->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder);encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache);Com<IWICBitmapFrameEncode> frame;Com<IPropertyBag2> props;encoder->CreateNewFrame(&frame,&props);frame->Initialize(props.Get());frame->SetSize(W,H);WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;frame->SetPixelFormat(&format);frame->WriteSource(bitmap.Get(),nullptr);frame->Commit();encoder->Commit();}
 auto original=fs::file_size(big);UINT w=0,h=0;if(!app.photoDimensions(big,w,h)||w!=W||h!=H)throw std::runtime_error("dimensions not read");
 auto small=folder/"small.jpg";if(!app.compressAccountPhoto(big,small,4096,.9f))throw std::runtime_error("compression failed");
 auto packed=fs::file_size(small);UINT sw=0,sh=0;if(!app.photoDimensions(small,sw,sh)||std::max(sw,sh)>4096||sw<16)throw std::runtime_error("compressed dimensions wrong");
 if(packed>=original||packed>App::accountPhotoLimit)throw std::runtime_error("compressed file not smaller than limit");
 atomicWrite(folder/"photo-result.json",json{{"ok",true},{"original",original},{"compressed",packed},{"width",sw},{"height",sh}}.dump(2));return 0;
 }catch(const std::exception& e){atomicWrite(folder/"photo-result.json",json{{"ok",false},{"error",e.what()}}.dump(2));return 1;}}
}
