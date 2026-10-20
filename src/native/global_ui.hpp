std::string modeAuthKey,modeAuthApi;
json accountProfile=json::object();
bool hardcore()const{return str(store->settings,"content_mode","standard")=="hardcore";}
void toggleContentMode(){{std::lock_guard lock(engine->mutex);if(engine->busy||!engine->tasks.empty()){status=L"생성 작업이 끝난 뒤 모드를 변경하세요.";return;}}save();if(!hardcore()){modeAuthKey=str(store->settings,"api_key");modeAuthApi=str(store->settings,"api");featureRequest("enable_hardcore","GET","/auth/info");return;}store->settings["content_mode"]="standard";selected.clear();preview.Reset();save();rebuild(false);scan();}
void globalSettingsPaint(Rect c){
 button({c.x+6,94,std::min(210.f,c.w-12),32},hardcore()?L"하드코어: 켜짐 · 끄기":L"하드코어: 꺼짐 · 켜기",[this]{toggleContentMode();},false,!engine->busy&&!learningBusy);
 label(wide(str(accountProfile,"role"))+L" · "+wide(str(accountProfile,"email")),{c.x+225,94,std::max(10.f,c.w-235),32},false,0xb0b0b7,11);
 if(!hwnd){float y=150;for(auto title:{L"생성 서버 주소",L"캡션 서버 주소"}){label(title,{c.x+6,y,140,27},false,0xb0b0b7,12);renderer.box({c.x+150,y,c.w-162,27},0x161617,0x3b3e48,3);label(L"설정 입력",{c.x+158,y,c.w-178,27},false,0x858590,12);y+=37;}label(L"스플래시 최소 2초",{c.x+6,y,140,25},false,0xb0b0b7,12);}
}
