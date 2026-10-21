std::string modeAuthKey,modeAuthApi;
json accountProfile=json::object();
bool hardcore()const{return str(store->settings,"content_mode","standard")=="hardcore";}
void accountLogoutMenu(){
 if(engine->busy||learningBusy||developerBusy||captionRunning)throw std::runtime_error("진행 중인 작업을 마친 뒤 로그아웃하세요.");
 HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,L"이 기기에서 로그아웃");AppendMenuW(menu,MF_STRING,2,L"모든 기기에서 로그아웃");POINT point;GetCursorPos(&point);int selected=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,hwnd,nullptr);DestroyMenu(menu);if(!selected)return;
 bool all=selected==2;if(all&&MessageBoxW(hwnd,L"다른 PC와 웹을 포함해 이 계정의 모든 로그인 세션을 종료합니다. 계속할까요?",L"전체 기기 로그아웃",MB_YESNO|MB_ICONWARNING)!=IDYES)return;
 featureRequest("account_logout","POST",accountLogoutPath(all));
}
void toggleContentMode(){{std::lock_guard lock(engine->mutex);if(engine->busy||!engine->tasks.empty()){status=L"생성 작업이 끝난 뒤 모드를 변경하세요.";return;}}save();if(!hardcore()){modeAuthKey=str(store->settings,"api_key");modeAuthApi=str(store->settings,"api");featureRequest("enable_hardcore","GET","/auth/info");return;}store->settings["content_mode"]="standard";selected.clear();preview.Reset();save();rebuild(false);scan();}
void globalSettingsPaint(Rect c){
 button({c.x+6,94,std::min(developer()?160.f:210.f,c.w-(developer()?104.f:12.f)),32},hardcore()?L"하드코어: 켜짐 · 끄기":L"하드코어: 꺼짐 · 켜기",[this]{toggleContentMode();},false,!engine->busy&&!learningBusy);
 if(developer())button({c.x+c.w-86,94,80,32},L"개발자",[this]{setPage(11);developerFetch();});
 if(!developer())label(wide(str(accountProfile,"role"))+L" · "+wide(str(accountProfile,"email")),{c.x+225,94,std::max(10.f,c.w-235),32},false,0xb0b0b7,11);
 if(!hwnd){float y=150;for(auto title:{L"서버 주소"}){label(title,{c.x+6,y,140,27},false,0xb0b0b7,12);renderer.box({c.x+150,y,c.w-162,27},0x161617,0x3b3e48,3);label(L"설정 입력",{c.x+158,y,c.w-178,27},false,0x858590,12);y+=37;}label(L"스플래시 최소 2초",{c.x+6,y,140,25},false,0xb0b0b7,12);}
}
