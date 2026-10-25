std::string modeAuthKey,modeAuthApi;
json accountProfile=json::object();
bool accountWorkspaceBusy(){std::lock_guard lock(engine->mutex);return engine->busy||!engine->tasks.empty()||engine->cancelling||learningBusy||developerBusy||captionRunning;}
void switchLocalAccount(const json& profile){save();clearControls();auto scope=profile.empty()?std::string():localAccountScope(str(store->settings,"api"),str(profile,"account_id"));store->useWorkspace(scope);store->settings["api_key"]=profile.empty()?"":accountCredential;store->settings["content_mode"]="standard";accountProfile=profile;quotaUsage=json::object();capabilities=json::object();selected.clear();paths.clear();preview.Reset();imageW=imageH=0;previewFailed=false;thumbs.clear();requested.clear();failedImages.clear();imageChecks.clear();images.reset();revision++;history=json::object();developerRows=json::array();developerView=0;developerOffset=0;serverAssetIds.clear();referenceProfiles.clear();learningProfiles=json::array();ollamaModels=json::array();archiveIndex=archiveOffset=0;archiveAll=archiveMore=false;refIndex=historyIndex=0;developerModeration={{"pre",false},{"post",false}};learnedInfo=json::object();archivedAssets=json::array();captionReference.clear();captionResultFile.clear();captionModels=json::array();learningNote.clear();learningAnalysisId.clear();trainingJob.clear();log.clear();status.clear();progress=0;modeAuthKey.clear();modeAuthApi.clear();accountPopover=false;projectsOpen=false;focusPreview=false;page=0;promptTab=std::clamp(store->state.value("prompt_tab",0),0,5);formScroll=scroll=projectScroll=0;loadWorkspace();welcome=profile.empty();store->save();}
void accountSignIn(bool creating=false){
 if(accountWorkspaceBusy())throw std::runtime_error("진행 중인 작업을 마친 뒤 로그인하세요.");
 save();if(!loginAccount(hwnd,store->baseRoot,store->settings,creating))return;
 NetworkScope scope(nullptr,std::chrono::steady_clock::now()+std::chrono::seconds(25));auto base=str(store->settings,"api");
 auto profile=request("GET",joinUrl(base,"/api/auth/me"),accountCredential).object();
 beforeDeviceRequest(joinUrl(base,"/api/usage"),accountCredential);
 auto usage=request("GET",joinUrl(base,"/api/usage"),accountCredential).object();switchLocalAccount(profile);quotaUsage=usage;
 welcome=false;page=0;save();status=L"로그인했습니다.";rebuild(false);scan();if(store->state["selected"].is_string()){auto path=fs::path(wide(store->state["selected"].get<std::string>()));if(fs::is_regular_file(path)&&localImageVisible(store->settings,path))select(path);}
}
void accountMenu(){if(accountProfile.empty()){accountSignIn();return;}setPage(13);}
void accountOverlay(){
 if(imageSearch)ShowWindow(imageSearch,accountPopover||referencePreviewMode()?SW_HIDE:SW_SHOW);if(!accountPopover)return;
 Rect panel{width-278,54,250,219};renderer.box(panel,0x20242f,0x41495e,12);icon({panel.x+16,panel.y+16,24,24},5,0xb5c2ff);
 label(wide(str(accountProfile,"email")),{panel.x+52,panel.y+11,182,24},true,0xe5e5e7,11);
 label(wide(str(accountProfile,"role")),{panel.x+52,panel.y+35,182,18},false,0x9ca7bf,10);renderer.line(panel.x+14,panel.y+65,panel.x+236,panel.y+65,0x3b4254);
 bool ready=!accountWorkspaceBusy();
 button({panel.x+12,panel.y+77,226,32},L"전체 설정",[this]{accountPopover=false;setPage(10);});
 button({panel.x+12,panel.y+119,226,32},L"로그아웃",[this]{accountPopover=false;featureRequest("account_logout","POST",accountLogoutPath(false));},false,ready);
 button({panel.x+12,panel.y+161,226,32},L"모든 기기에서 로그아웃",[this]{accountPopover=false;if(MessageBoxW(hwnd,L"이 계정의 모든 로그인 세션을 종료할까요?",L"모든 기기에서 로그아웃",MB_YESNO|MB_ICONQUESTION)==IDYES)featureRequest("account_logout","POST",accountLogoutPath(true));},false,ready);
}
bool hardcore()const{return str(store->settings,"content_mode","standard")=="hardcore";}
void toggleContentMode(){{std::lock_guard lock(engine->mutex);if(engine->busy||!engine->tasks.empty()){status=L"생성 작업이 끝난 뒤 모드를 변경하세요.";return;}}save();if(!hardcore()){modeAuthKey=str(store->settings,"api_key");modeAuthApi=str(store->settings,"api");featureRequest("enable_hardcore","GET","/auth/info");return;}store->settings["content_mode"]="standard";selected.clear();preview.Reset();save();rebuild(false);scan();}
void globalSettingsPaint(Rect c){
 button({c.x+6,94,std::min(developer()?160.f:210.f,c.w-(developer()?104.f:12.f)),32},hardcore()?L"하드코어 ON":L"하드코어 OFF",[this]{toggleContentMode();},false,!engine->busy&&!learningBusy);

 if(!hwnd){float y=150;for(auto title:{L"앱 설정"}){label(title,{c.x+6,y,140,27},false,0xb0b0b7,12);renderer.box({c.x+150,y,c.w-162,27},0x161617,0x3b3e48,3);label(L"설정 입력",{c.x+158,y,c.w-178,27},false,0x858590,12);y+=37;}label(L"스플래시 최소 2초",{c.x+6,y,140,25},false,0xb0b0b7,12);}
}
