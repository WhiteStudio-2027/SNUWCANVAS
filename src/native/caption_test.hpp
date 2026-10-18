#pragma once
#include "caption_api.hpp"
namespace snuw {
inline int captionTest(const fs::path& folder,const std::string& base,const std::string& mode){try{
 json cfg={{"caption_api",base},{"caption_api_key","caption-test-secret"},{"caption_model","vision:model/8b"},{"caption_format","tags"}};std::atomic_bool cancel{false};auto begin=std::chrono::steady_clock::now();
 auto models=captionRequest(cfg,{},true,&cancel);if(models.value("models",json::array()).empty())throw std::runtime_error("Missing vision models");
 bool interrupted=false;std::thread interrupter;if(mode=="cancel")interrupter=std::thread([&]{Sleep(100);cancel=true;});json result;
 try{result=captionRequest(cfg,folder/L"input.png",false,&cancel);}catch(...){if(mode!="cancel"){if(interrupter.joinable())interrupter.join();throw;}interrupted=true;}
 if(interrupter.joinable())interrupter.join();if(mode=="cancel"&&!interrupted)throw std::runtime_error("Caption request did not cancel");if(mode!="cancel"&&(str(result,"text")!="solo, blue hair"||str(result,"negative_prompt")!="duplicate subject, blurry"))throw std::runtime_error("Caption response mismatch");
 auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();if(mode=="cancel"&&seconds>2)throw std::runtime_error("Caption cancellation blocked");atomicWrite(folder/"caption-result.json",json{{"ok",true},{"cancelled",interrupted},{"gui_opened",false}}.dump(2));return 0;
 }catch(const std::exception& e){atomicWrite(folder/"caption-result.json",json{{"ok",false},{"error",e.what()}}.dump(2));return 1;}}
}
