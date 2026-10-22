#pragma once
#include "app.hpp"
namespace snuw {
inline int studioTest(const fs::path& folder){try{
 INITCOMMONCONTROLSEX controls{sizeof controls,ICC_STANDARD_CLASSES|ICC_WIN95_CLASSES};InitCommonControlsEx(&controls);
 if(!LoadLibraryW(L"Msftedit.dll"))throw std::runtime_error("Rich text controls unavailable");
 auto store=std::make_shared<Store>(folder);store->load();App app(store);app.accountProfile={{"email","test@example.invalid"},{"role","safe"}};
 WNDCLASSW klass{};klass.hInstance=GetModuleHandle(nullptr);klass.lpfnWndProc=DefWindowProcW;klass.lpszClassName=L"SnuwStudioTest";RegisterClassW(&klass);
 HWND window=CreateWindowExW(0,klass.lpszClassName,L"",WS_POPUP,0,0,1320,900,nullptr,nullptr,klass.hInstance,nullptr);
 if(!window)throw std::runtime_error("Hidden studio window failed");app.init(window);int viewports=0;
 for(auto dimensions:std::vector<std::pair<int,int>>{{1040,681},{1040,720},{1320,900},{1920,1080}}){
  SetWindowPos(window,nullptr,0,0,int(dimensions.first*app.dpi),int(dimensions.second*app.dpi),SWP_NOZORDER|SWP_NOACTIVATE);app.resize();auto panel=app.center(),card=app.card();
  if(panel.w>381||panel.w<300||(dimensions.first>=1320&&app.rightWidth<=panel.w*1.5f))throw std::runtime_error("Editor is not compact relative to image canvas");
  for(auto input:{app.positive,app.negative}){RECT bounds{};if(!input||!GetWindowRect(input,&bounds))throw std::runtime_error("Prompt control was not created");MapWindowPoints(nullptr,window,(POINT*)&bounds,2);if(bounds.left<panel.x*app.dpi||bounds.right>(panel.x+panel.w)*app.dpi||bounds.bottom>(card.y-60)*app.dpi)throw std::runtime_error("Prompt input overlaps the generation card");}
  app.paint();if(!app.status.empty())throw std::runtime_error(utf8(app.status));if(app.browser.h<app.height*.55f)throw std::runtime_error("Gallery is too small");
  app.focusPreview=true;app.paint();if(app.previewRect.h<app.height*.55f||app.browser.h!=0)throw std::runtime_error("Large preview mode failed");app.focusPreview=false;viewports++;
 }
 app.focusPreview=false;app.select(folder/"missing-preview-fixture.png");if(!app.focusPreview)throw std::runtime_error("Selecting image did not enable preview");app.selected.clear();app.focusPreview=false;
 auto normal=store->settings;normal["steps"]="31";normal["width"]="832";normal["height"]="1216";normal["_previous_workflow"]="기존 SDXL";normal["workflow_profile"]="픽셀아트 · 8× 격자";if(!switchPixelSettings(normal)||modelId(normal)!="pixel_art"||str(normal,"width")!="1024")throw std::runtime_error("Pixel defaults missing");auto pixel=snuw::controls(normal);if(pixel["checkpoint"]!="sd_xl_base_1.0_0.9vae.safetensors"||pixel["hires_enabled"]!=false)throw std::runtime_error("Pixel model isolation missing");normal["workflow_profile"]="기존 SDXL";if(!switchPixelSettings(normal)||str(normal,"steps")!="31"||str(normal,"width")!="832")throw std::runtime_error("Normal settings not restored");
 if(!app.navTooltip||SendMessageW(app.navTooltip,TTM_GETTOOLCOUNT,0,0)!=app.navigationPages().size()+6)throw std::runtime_error("Navigation descriptions missing");int before=app.page;app.message(WM_MOUSEMOVE,0,MAKELPARAM(24,124));if(app.page!=before)throw std::runtime_error("Hover changed the page");app.click(24,124);if(app.page!=1)throw std::runtime_error("Sidebar click did not change the page");
 app.setPage(10);app.paint();for(auto& f:app.fields)if(f.key=="workflow_profile"||f.key=="genre"||f.key=="render_mode")throw std::runtime_error("Global settings retained generation controls");
 app.switchProject(store->active);app.paint();if(app.page!=0||!app.positive||!app.negative)throw std::runtime_error("Channel selection did not open generation");
 auto savedSettings=store->settings;store->settings["workflow_profile"]="픽셀아트";store->settings["pixel_width"]="64";store->settings["pixel_height"]="128";app.rebuild(false);bool pw=false,ph=false;for(auto& f:app.fields){if(f.key=="pixel_width")pw=IsWindowEnabled(f.hwnd);if(f.key=="pixel_height")ph=IsWindowEnabled(f.hwnd);}if(!pw||!ph||!modelLocked(store->settings,"steps")||!modelLocked(store->settings,"cfg"))throw std::runtime_error("Pixel resolution controls or fixed settings missing");auto c=snuw::controls(store->settings);if(c["pixel_width"]!=64||c["pixel_height"]!=128||c["steps"]!=28)throw std::runtime_error("Pixel output settings not transmitted");store->settings=savedSettings;app.rebuild(false);
 app.accountProfile={{"email","test@example.invalid"},{"role","safe"}};store->settings["api_key"]=accountCredential;
 app.accountMenu();app.paint();if(app.page!=13||!app.profileNickname||!app.profileBio)throw std::runtime_error("Account page did not open");app.setPage(0);app.paint();
 for(auto& entry:app.navigationPages())if(entry.first==7||entry.first==8)throw std::runtime_error("Regular account can see developer pages");bool denied=false;try{app.setPage(7);}catch(...){denied=true;}if(!denied)throw std::runtime_error("Regular account opened consistency");
 app.engine->event({"progress",store->active,"running",{{"count",2},{"completed",0},{"progress",{{"stage","sampling"},{"percent",42}}}}});app.tick();if(app.status.find(L"42%")==std::wstring::npos||app.progress<20||app.progress>22)throw std::runtime_error("Live sample progress was not shown");app.engine->event({"transfer",store->active,"이미지 다운로드 중",{}});app.tick();if(app.status.find(L"다운로드")==std::wstring::npos)throw std::runtime_error("Download phase not visible");
 app.learningReply({"learning_reply",store->active,"account_logout",json::object()});
 if(!app.accountProfile.empty()||!str(store->settings,"api_key").empty()||!IsWindow(window)||app.closing||!app.welcome||app.positive||app.negative)throw std::runtime_error("Logout closed the workspace or retained credentials");
 bool locked=false;try{app.enqueue();}catch(...){locked=true;}if(!locked)throw std::runtime_error("Logged-out workspace allowed generation");app.paint();if(app.buttons.size()!=2)throw std::runtime_error("Logged-out screen exposed workspace actions");
 app.engine->stopping=true;app.engine->stop();DestroyWindow(window);
 atomicWrite(folder/"studio-result.json",json{{"ok",true},{"gui_opened",false},{"viewports",viewports},{"logout_keeps_workspace",true},{"large_preview",true},{"account_page",true}}.dump(2));return 0;
 }catch(const std::exception& error){atomicWrite(folder/"studio-result.json",json{{"ok",false},{"error",error.what()}}.dump(2));return 1;}}
}
