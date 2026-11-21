#pragma once
#include "app.hpp"
namespace snuw {
inline int localBuildWatchTest(const fs::path& folder){try{
 auto store=std::make_shared<Store>(folder/"state");store->load();store->settings["auto_update"]=false;store->state["drafts"]["prompt"]="local build preserves this draft";
 App app(store);app.hwnd=CreateWindowExW(0,L"STATIC",L"",WS_POPUP,0,0,100,100,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);if(!app.hwnd)throw std::runtime_error("Test window unavailable");
 app.localBuildRoot=fs::absolute(folder);app.localBuildExecutable=app.localBuildRoot/"SNUWCANVAS.exe";app.localBuildRunningHash=fileSha256(app.localBuildExecutable);{std::lock_guard lock(app.engine->mutex);app.engine->busy=true;}
 if(fs::exists(folder/"expect-rejected")){auto until=GetTickCount64()+4000;while(GetTickCount64()<until){app.localBuildTick(true);for(auto& event:app.engine->take())if(event.kind=="local_build_ready"||event.kind=="local_build_error")app.localBuildReply(event);if(!app.localBuildPending.empty())throw std::runtime_error("An older build was queued");Sleep(20);}if(app.localBuildThread.joinable())app.localBuildThread.join();DestroyWindow(app.hwnd);app.hwnd=nullptr;app.engine->stopping=true;app.engine->cv.notify_all();delete app.engine;app.engine=nullptr;atomicWrite(folder/"watch-result.json",json{{"ok",true},{"rejected",true}}.dump());return 0;}
 auto deadline=GetTickCount64()+10000;while(GetTickCount64()<deadline&&app.localBuildPending.empty()){app.localBuildTick(true);for(auto& event:app.engine->take())if(event.kind=="local_build_ready"||event.kind=="local_build_error")app.localBuildReply(event);Sleep(20);}
 if(app.localBuildPending.empty())throw std::runtime_error("Watcher did not detect the staged build");if(app.localBuildTick(true)||app.localBuildRestarting)throw std::runtime_error("Watcher restarted during generation");
 if(fileSha256(app.localBuildExecutable)!=app.localBuildRunningHash)throw std::runtime_error("Busy executable changed");{std::lock_guard lock(app.engine->mutex);app.engine->busy=false;}
 if(!app.localBuildTick(true)||!app.localBuildRestarting)throw std::runtime_error("Idle watcher did not schedule restart");Store reload(folder/"state");reload.load();if(str(reload.state["drafts"],"prompt")!="local build preserves this draft")throw std::runtime_error("Restart lost draft");
 if(app.localBuildThread.joinable())app.localBuildThread.join();DestroyWindow(app.hwnd);app.hwnd=nullptr;app.engine->stopping=true;app.engine->cv.notify_all();delete app.engine;app.engine=nullptr;
 atomicWrite(folder/"watch-result.json",json{{"ok",true},{"busy_deferred",true},{"draft_saved",true},{"gui_opened",false}}.dump());return 0;
 }catch(const std::exception& error){atomicWrite(folder/"watch-result.json",json{{"ok",false},{"error",error.what()}}.dump());return 1;}}
}
