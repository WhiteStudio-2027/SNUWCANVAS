#pragma once
#include "app.hpp"
namespace snuw {
inline int controlTest(const fs::path& folder,const std::string& base,const std::string& mode){try{
 auto cfg=defaults(folder);cfg.update({{"api",base},{"api_key","control-key"},{"count","2"},{"width","832"},{"height","1216"},{"job_timeout_seconds",mode=="timeout"?"1":"30"}});auto state=newState(cfg);state["drafts"]["prompt"]="test";Engine engine;engine.enqueue({"control",id(),cfg,state,false});
 if(mode=="poll"||mode=="submit"||mode=="download"){engine.enqueue({"queued",id(),cfg,state,false});auto readyUntil=std::chrono::steady_clock::now()+std::chrono::seconds(8);while(!fs::exists(folder/"network-started")&&std::chrono::steady_clock::now()<readyUntil)Sleep(10);if(!fs::exists(folder/"network-started"))throw std::runtime_error("Fixture request did not start");engine.stop();}
 auto started=std::chrono::steady_clock::now();bool stopped=false,confirmed=false,error=false;std::string reason;
 for(int i=0;i<400;i++){for(auto& event:engine.take()){if(event.kind=="error"){error=true;reason=event.text;}if(event.kind=="cancelled")confirmed=event.data.value("confirmed",false);}if(!engine.busy&&!engine.cancelling){std::lock_guard lock(engine.mutex);if(engine.tasks.empty()){stopped=true;break;}}Sleep(10);}
 auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();if(!stopped||!confirmed)throw std::runtime_error("Cancellation did not complete: "+reason);if((mode=="poll"||mode=="submit"||mode=="download")&&elapsed>2000)throw std::runtime_error("Local stop blocked on network");if((mode=="timeout"||mode=="missing")&&!error)throw std::runtime_error("Failure was not reported");
 auto store=std::make_shared<Store>(folder/"click");store->load();App app(store);int clicks=0;app.buttons.push_back({{10,10,60,30},L"test",[&]{clicks++;},false});app.click(20,20);app.message(WM_LBUTTONDBLCLK,0,MAKELPARAM(20,20));if(clicks!=1)throw std::runtime_error("Double click submitted twice");
 atomicWrite(folder/"native-control-result.json",json{{"ok",true},{"confirmed",confirmed},{"stop_ms",elapsed},{"double_click_submissions",clicks},{"gui_opened",false}}.dump(2));return 0;
 }catch(const std::exception& e){atomicWrite(folder/"native-control-result.json",json{{"ok",false},{"error",e.what()},{"gui_opened",false}}.dump(2));return 1;}}
}
