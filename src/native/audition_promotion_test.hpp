#pragma once
namespace snuw {
inline int auditionPromotionTest(const fs::path& folder,const std::string& url){try{
 LoadLibraryW(L"Msftedit.dll");INITCOMMONCONTROLSEX init{sizeof init,ICC_STANDARD_CLASSES};InitCommonControlsEx(&init);
 auto store=std::make_shared<Store>(folder/"state");store->load();store->useWorkspace(std::string(64,'a'));store->settings["auto_update"]=false;store->settings["api"]=url;store->settings["api_key"]="test-key";store->settings["channel_id"]=store->active;store->save();
 auto bytes=readFile(folder/"fixture.png");auto path=fs::path(wide(str(store->settings,"output")))/"candidate.png";atomicWrite(path,bytes);registerLocalImage(store->settings,path);
 App app(store);app.accountProfile={{"role","develop"}};auto parent=CreateWindowExW(0,L"STATIC",L"",0,0,0,1000,800,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);if(!parent)throw std::runtime_error("Hidden promotion parent failed");app.hwnd=parent;
 auto& ledger=app.auditions();auto run=ledger.create(json::array({{{"id",id()},{"snapshot",panelSnapshot(store->state)}}}),2,url,"standard");ledger.image(run,0,utf8(path.wstring()),sha256(bytes));ledger.image(run,1,utf8(path.wstring()));app.auditionRun=run;app.auditionSelected=0;
 auto denied=[&](auto fn){bool blocked=false;try{fn();}catch(...){blocked=true;}if(!blocked)throw std::runtime_error("Unsafe promotion accepted");};
 denied([&]{app.auditionPromotionCandidate();});ledger.rate(run,0,5);ledger.choose(run,0);auto source=app.auditionPromotionCandidate();
 store->settings["content_mode"]="hardcore";denied([&]{app.auditionPromotionCandidate();});store->settings["content_mode"]="standard";
 store->settings["api"]=url+"/other";denied([&]{app.auditionPromotionCandidate();});store->settings["api"]=url;
 app.auditionSelected=1;ledger.rate(run,1,5);ledger.choose(run,1);denied([&]{app.auditionPromotionCandidate();});ledger.choose(run,0);app.auditionSelected=0;
 auto foreign=store->root.parent_path()/std::string(64,'b')/"foreign.png";atomicWrite(foreign,bytes);auto original=str(ledger.task(run,0),"path");ledger.task(run,0)["path"]=utf8(foreign.wstring());denied([&]{app.auditionPromotionCandidate();});ledger.task(run,0)["path"]=original;
 app.promoteAuditionReference("character");auto refs=store->state["project"]["references"];if(refs.size()!=1||str(refs[0],"audition_sha256")!=sha256(bytes)||refs[0]["audition_source"]!=source)throw std::runtime_error("Promotion provenance lost");denied([&]{app.promoteAuditionReference("style");});
 auto cfg=store->settings;cfg["count"]="1";generationForm(cfg,store->state,id());cfg["content_mode"]="hardcore";denied([&]{generationForm(cfg,store->state,id());});cfg["content_mode"]="standard";atomicWrite(path,bytes+"changed");denied([&]{generationForm(cfg,store->state,id());});
 auto profile=std::string(32,'c');app.learningProfiles=json::array({{{"id",profile},{"name","Target"},{"kind","character"}},{{"id",std::string(32,'d')},{"name","Text"},{"kind","object"}}});denied([&]{app.promoteAuditionTraining(std::string(32,'d'));});
 app.accountProfile["role"]="safe";denied([&]{app.promoteAuditionTraining(profile);});app.accountProfile["role"]="develop";
 auto pump=[&]{for(int i=0;i<600&&app.referenceUploadRunning;i++){app.tick();Sleep(5);}if(app.referenceUploadRunning||app.learningBusy)throw std::runtime_error("Promotion worker stalled");};
 store->settings["profile_caption"]="Preserved caption draft";app.promoteAuditionTraining(profile);pump();if(app.learningNote.find(L"변경") == std::wstring::npos)throw std::runtime_error("Changed training image was not rejected");
 atomicWrite(path,bytes);app.promoteAuditionTraining(profile);pump();if(str(store->settings,"profile_caption")!="Preserved caption draft"||store->state["project"]["references"]!=refs||readFile(path)!=bytes)throw std::runtime_error("Promotion altered originals");
 if(app.referenceTransferThread.joinable())app.referenceTransferThread.join();app.clearControls();DestroyWindow(parent);app.hwnd=nullptr;app.engine->stopping=true;app.engine->cv.notify_all();delete app.engine;app.engine=nullptr;
 atomicWrite(folder/"audition-promotion-result.json",json{{"ok",true},{"scope_mode_server",true},{"selected_only",true},{"file_integrity",true},{"provenance",true},{"explicit_upload",true}}.dump());return 0;
 }catch(const std::exception& e){atomicWrite(folder/"audition-promotion-result.json",json{{"ok",false},{"error",e.what()}}.dump());return 1;}}
}
