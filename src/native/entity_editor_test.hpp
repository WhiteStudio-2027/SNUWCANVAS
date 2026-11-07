#pragma once
#include "app.hpp"
namespace snuw {
inline int entityEditorTest(const fs::path& folder,const std::string& base,bool unavailable){try{
 for(auto text:{L"12:30",L"12:30:00",L"https:",L"https://example/x:하",L"\\:하루:",L":하루:"})if(entityPrefix(text).valid)throw std::runtime_error("Literal clock/URL/escaped/closed token offered completion");
 for(auto text:{L":",L":하",L"오늘:하루",L"그림: :하",L":하루::"})if(!entityPrefix(text).valid)throw std::runtime_error("Valid partial token was not offered completion");
 json tricky=json::array({{{"id",std::string(32,'a')},{"name","Webtoon"}},{{"id",std::string(32,'b')},{"name","Same"}},{{"id",std::string(32,'c')},{"name","Ｓａｍｅ"}},{{"id",std::string(32,'d')},{"name","123"}},{{"id",std::string(32,'e')},{"name","bad:name"}}});
 if(matchingEntities(tricky,L"ｗｅｂ")!=std::vector<std::wstring>{L"Webtoon"}||matchingEntities(tricky,L"").size()!=1)throw std::runtime_error("Unicode search or ambiguous/invalid name filtering failed");
 LoadLibraryW(L"Msftedit.dll");INITCOMMONCONTROLSEX init{sizeof init,ICC_STANDARD_CLASSES};InitCommonControlsEx(&init);auto store=std::make_shared<Store>(folder/"state");store->load();store->settings["api"]=base;store->settings["api_key"]="entity-test-key";
 App app(store);app.accountProfile={{"role","develop"}};auto parent=CreateWindowExW(0,L"STATIC",L"",0,0,0,600,400,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);if(!parent)throw std::runtime_error("Hidden parent failed");app.hwnd=parent;app.page=0;
 app.positive=app.control(L"EDIT",L"",ES_MULTILINE|ES_AUTOVSCROLL,110);if(!app.positive)throw std::runtime_error("Hidden edit failed");app.promptDisplayed=normalizedText(app.positive);
 auto text=[&](const std::wstring& value,LONG caret=-1){SetWindowTextW(app.positive,value.c_str());SendMessageW(app.positive,EM_SETSEL,caret,caret);app.sync();};
 text(L"PRIVATE_INPUT :하");app.loadEntityNames();auto until=GetTickCount64()+8000;
 while(app.entityRunning&&GetTickCount64()<until){for(auto& event:app.engine->take())if(event.kind=="entity_names")app.entityNamesReply(event);Sleep(10);}if(app.entityRunning||!app.entityLoaded)throw std::runtime_error("Name request did not complete");if(app.entityThread.joinable())app.entityThread.join();
 if(unavailable){if(!app.entityNames.empty()||app.entityRefreshAt<=GetTickCount64())throw std::runtime_error("Unavailable name service was not quiet/backed off");}
 else {
  auto matching=matchingEntities(app.entityNames,L"하");if(matching!=std::vector<std::wstring>{L"하루"})throw std::runtime_error("Name response/filter mismatch");
  auto prefix=entityPrefix(L"PRIVATE_INPUT :하");app.showEntitySuggestions(prefix,LONG(std::wstring(L"PRIVATE_INPUT :하").size()),matching);if(!app.entityList)throw std::runtime_error("Completion widget was not created");app.acceptEntity(0);if(normalizedText(app.positive)!="PRIVATE_INPUT :하루:")throw std::runtime_error("Completion insertion failed");app.undoPrompt();if(normalizedText(app.positive)!="PRIVATE_INPUT :하")throw std::runtime_error("Completion undo failed");
  text(L"PRIVATE_INPUT :하:",LONG(std::wstring(L"PRIVATE_INPUT :하").size()));app.showEntitySuggestions(prefix,LONG(std::wstring(L"PRIVATE_INPUT :하").size()),matching);app.acceptEntity(0);if(normalizedText(app.positive)!="PRIVATE_INPUT :하루:")throw std::runtime_error("Existing closing colon was duplicated");
  text(L"PRIVATE_INPUT :하");app.showEntitySuggestions(prefix,LONG(std::wstring(L"PRIVATE_INPUT :하").size()),matching);SetWindowTextW(app.positive,L"PRIVATE_INPUT :다");SendMessageW(app.positive,EM_SETSEL,-1,-1);app.acceptEntity(0);if(normalizedText(app.positive)!="PRIVATE_INPUT :다")throw std::runtime_error("Stale completion overwrote changed text");app.sync();
  auto previous=app.entityCacheScope;store->workspaceScope=std::string(64,'b');app.entityNamesReply({"entity_names","","",{{"scope",previous},{"names",json::array({{{"id",std::string(32,'f')},{"name","WRONG_ACCOUNT"}}})}}});if(!app.entityNames.empty())throw std::runtime_error("Late account response was retained");
 }
 app.hideEntitySuggestions();DestroyWindow(parent);app.positive=nullptr;app.hwnd=nullptr;app.engine->stopping=true;app.engine->cv.notify_all();if(app.engine->worker.joinable())app.engine->worker.join();delete app.engine;app.engine=nullptr;
 atomicWrite(folder/"entity-editor-result.json",json{{"ok",true},{"gui_opened",false},{"unavailable",unavailable},{"private_prefix_sent",false}}.dump(2));return 0;
 }catch(const std::exception& e){atomicWrite(folder/"entity-editor-result.json",json{{"ok",false},{"gui_opened",false},{"error",e.what()}}.dump(2));return 1;}}
}
