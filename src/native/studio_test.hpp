#pragma once
#include "app.hpp"
namespace snuw {
inline int studioTest(const fs::path& folder){try{
 INITCOMMONCONTROLSEX controls{sizeof controls,ICC_STANDARD_CLASSES|ICC_WIN95_CLASSES};InitCommonControlsEx(&controls);
 if(!LoadLibraryW(L"Msftedit.dll"))throw std::runtime_error("Rich text controls unavailable");
 auto store=std::make_shared<Store>(folder);store->load();App app(store);
 WNDCLASSW klass{};klass.hInstance=GetModuleHandle(nullptr);klass.lpfnWndProc=DefWindowProcW;klass.lpszClassName=L"SnuwStudioTest";RegisterClassW(&klass);
 HWND window=CreateWindowExW(0,klass.lpszClassName,L"",WS_POPUP,0,0,1320,900,nullptr,nullptr,klass.hInstance,nullptr);
 if(!window)throw std::runtime_error("Hidden studio window failed");app.init(window);int viewports=0;
 for(auto dimensions:std::vector<std::pair<int,int>>{{1040,681},{1040,720},{1320,900},{1920,1080}}){
  SetWindowPos(window,nullptr,0,0,dimensions.first,dimensions.second,SWP_NOZORDER|SWP_NOACTIVATE);app.resize();auto panel=app.center(),card=app.card();
  if(panel.w>381||panel.w<300||(dimensions.first>=1320&&app.rightWidth<=panel.w*1.5f))throw std::runtime_error("Editor is not compact relative to image canvas");
  for(auto input:{app.positive,app.negative}){RECT bounds{};if(!input||!GetWindowRect(input,&bounds))throw std::runtime_error("Prompt control was not created");MapWindowPoints(nullptr,window,(POINT*)&bounds,2);if(bounds.left<panel.x*app.dpi||bounds.right>(panel.x+panel.w)*app.dpi||bounds.bottom>(card.y-60)*app.dpi)throw std::runtime_error("Prompt input overlaps the generation card");}
  app.paint();if(!app.status.empty())throw std::runtime_error(utf8(app.status));if(app.browser.h<app.height*.55f)throw std::runtime_error("Gallery is too small");
  app.focusPreview=true;app.paint();if(app.previewRect.h<app.height*.55f||app.browser.h!=0)throw std::runtime_error("Large preview mode failed");app.focusPreview=false;viewports++;
 }
 app.accountProfile={{"email","test@example.invalid"},{"role","safe"}};store->settings["api_key"]=accountCredential;
 app.accountMenu();app.paint();if(!app.accountPopover)throw std::runtime_error("Account popover did not open");app.click(0,0);if(app.accountPopover)throw std::runtime_error("Account popover did not close outside");
 app.learningReply({"learning_reply",store->active,"account_logout",json::object()});
 if(!app.accountProfile.empty()||!str(store->settings,"api_key").empty()||!IsWindow(window)||app.closing)throw std::runtime_error("Logout closed the workspace or retained credentials");
 app.engine->stopping=true;app.engine->stop();DestroyWindow(window);
 atomicWrite(folder/"studio-result.json",json{{"ok",true},{"gui_opened",false},{"viewports",viewports},{"logout_keeps_workspace",true},{"large_preview",true},{"account_popover",true}}.dump(2));return 0;
 }catch(const std::exception& error){atomicWrite(folder/"studio-result.json",json{{"ok",false},{"error",error.what()}}.dump(2));return 1;}}
}
