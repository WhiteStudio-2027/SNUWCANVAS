std::string modeAuthKey,modeAuthApi;
bool hardcore()const{return str(store->settings,"content_mode","standard")=="hardcore";}
void toggleContentMode(){{std::lock_guard lock(engine->mutex);if(engine->busy||!engine->tasks.empty()){status=L"생성 작업이 끝난 뒤 모드를 변경하세요.";return;}}save();if(!hardcore()&&str(store->settings,"api_key").rfind("fr-",0)!=0){status=L"하드코어 모드는 fr- 권한 키가 필요합니다.";return;}if(!hardcore()){modeAuthKey=str(store->settings,"api_key");modeAuthApi=str(store->settings,"api");featureRequest("enable_hardcore","GET","/auth/info");return;}store->settings["content_mode"]="standard";selected.clear();preview.Reset();save();rebuild(false);scan();}
void globalSettingsPaint(Rect c){bool hard=hardcore();Rect r{c.x+10,94,c.w-20,198};renderer.box(r,hard?0x150d0e:0x16171b,hard?0x963e40:0x3a3c43,16);
 label(hard?L"HARDCORE MODE":L"STANDARD MODE",{r.x+16,r.y+12,r.w-100,26},true,hard?0xffb7b7:0xe5e5e7,16);
 label(hard?L"성인 콘텐츠 제한 완화":L"기본 콘텐츠 필터 적용 중",{r.x+16,r.y+41,r.w-32,24},false,hard?0xe3a2a2:0xaaaab4,12);
 Rect toggle{r.x+r.w-70,r.y+18,52,28};renderer.box(toggle,hard?0x943b40:0x3d4049,0,14);renderer.brush->SetColor(color(0xffffff));renderer.target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(toggle.x+(hard?38:14),toggle.y+14),11,11),renderer.brush.Get());buttons.push_back({toggle,L"",[this]{toggleContentMode();},false,!engine->busy});
 renderer.line(r.x+14,r.y+76,r.x+r.w-14,r.y+76,hard?0x673438:0x393c43);
 renderer.box({r.x+14,r.y+90,r.w-28,60},hard?0x32181a:0x262930,0,10);
 label(hard?L"콘텐츠 경고":L"콘텐츠 보호 활성화",{r.x+26,r.y+94,r.w-52,26},true,hard?0xff9696:0xd4d8e1,14);
 label(hard?L"민감한 콘텐츠가 포함될 수 있습니다.":L"민감한 콘텐츠가 기본적으로 제한됩니다.",{r.x+26,r.y+122,r.w-52,22},false,hard?0xe0aaaa:0xb9bec8,11);
 button({r.x+14,r.y+162,r.w-28,27},hard?L"일반 모드로 돌아가기":L"하드코어 모드 활성화",[this]{toggleContentMode();},false,!engine->busy);
 if(!hwnd){float y=310;for(auto title:{L"생성 서버 주소",L"생성 API 키",L"캡션 서버 주소",L"캡션 API 키"}){label(title,{c.x+6,y,140,27},false,0xb0b0b7,12);renderer.box({c.x+150,y,c.w-162,27},0x161617,0x3b3e48,3);label(L"설정 입력",{c.x+158,y,c.w-178,27},false,0x858590,12);y+=37;}label(L"스플래시 최소 2초",{c.x+6,y,140,25},false,0xb0b0b7,12);}
}
