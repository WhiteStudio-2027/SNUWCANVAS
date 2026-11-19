bool welcome=false,projectsOpen=false;
Com<ID2D1Bitmap> welcomeBackground,welcomeLogo;
bool welcomeBackgroundChecked=false;
std::vector<Com<ID2D1Bitmap>> welcomePanorama;
json welcomePanoramaEntries=json::array();size_t welcomePanoramaCursor=0;
bool welcomePersonalPanorama=false,welcomePanoramaPaused=false,welcomeMotionPreferenceLoaded=false,welcomeSystemMotion=true;
double welcomePanoramaLast=0,welcomePanoramaSeconds=0;bool welcomePanoramaTimer=false;
static constexpr double panoramaSpeed=30.;
double panoramaNowSeconds()const{return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void updateWelcomeTimer(bool needed){if(!hwnd)return;if(needed&&!welcomePanoramaTimer)welcomePanoramaTimer=SetTimer(hwnd,5,16,nullptr)!=0;else if(!needed&&welcomePanoramaTimer){KillTimer(hwnd,5);welcomePanoramaTimer=false;}}
void advanceWelcomePanorama(double now,bool moving){if(moving&&welcomePanoramaLast>0)welcomePanoramaSeconds+=std::max(0.,now-welcomePanoramaLast);welcomePanoramaLast=now;}
void resetWelcomeGraphics(){welcomeBackground.Reset();welcomeLogo.Reset();welcomePanorama.clear();welcomePanoramaEntries=json::array();welcomePanoramaCursor=0;welcomeBackgroundChecked=false;welcomePanoramaLast=0;updateWelcomeTimer(false);}
bool welcomeMotionAllowed(){if(!welcomeMotionPreferenceLoaded){BOOL enabled=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&enabled,0);welcomeSystemMotion=enabled!=FALSE;welcomeMotionPreferenceLoaded=true;}return welcomeSystemMotion&&!welcomePanoramaPaused;}
double panoramaOffsetFor(double seconds,double cycle)const{return cycle>0?std::fmod(std::max(0.,seconds)*panoramaSpeed,cycle):0.;}
void loadApprovedWelcomeBackground(){
 try{
  auto bytes=[](int key,DWORD limit){auto resource=FindResourceW(nullptr,MAKEINTRESOURCEW(key),RT_RCDATA);if(!resource)return std::string{};auto size=SizeofResource(nullptr,resource);if(!size||size>limit)return std::string{};auto data=LockResource(LoadResource(nullptr,resource));return data?std::string(static_cast<const char*>(data),size):std::string{};};
  if(!welcomeBackgroundChecked){welcomeBackgroundChecked=true;auto manifest=json::parse(bytes(205,65536));if(!manifest.is_object()||manifest.value("schema",0)!=1||str(manifest,"motion")!="horizontal_loop"||!manifest.contains("entries")||!manifest["entries"].is_array()||manifest["entries"].size()>16)return;welcomePersonalPanorama=flag(manifest,"personal_background");welcomePanoramaEntries=manifest["entries"];}
  // Decode at most one image per frame; keep already loaded textures for drawing.
  if(welcomePanoramaCursor<welcomePanoramaEntries.size()){
   const auto& entry=welcomePanoramaEntries[welcomePanoramaCursor++];
   if(!entry.is_object()||(!flag(entry,"safe_approved")&&!(welcomePersonalPanorama&&flag(entry,"selected_by_user")))||!flag(entry,"rights_approved")||str(entry,"source").empty()||str(entry,"license").empty())return;
   if(!entry.contains("resource")||!entry["resource"].is_number_integer())return;int resource=entry["resource"].get<int>();if(resource<230||resource>239)return;
   auto expected=str(entry,"sha256");if(expected.size()!=64||expected.find_first_not_of("0123456789abcdef")!=std::string::npos)return;
   auto image=bytes(resource,25*1024*1024);if(image.empty()||sha256(image)!=expected)return;
   auto bitmap=renderer.resource(resource);if(!bitmap)return;auto size=bitmap->GetSize();if(size.width<1||size.height<1||size.width>16384||size.height>16384)return;
   welcomePanorama.push_back(bitmap);if(!welcomeBackground)welcomeBackground=bitmap;
  }
 }catch(...){/* A bad entry is omitted; valid loaded images remain available. */}
}
void paintWelcomePanorama(){
 loadApprovedWelcomeBackground();double cycle=0;for(auto& bitmap:welcomePanorama){auto size=bitmap->GetSize();cycle+=height*size.width/size.height;}
 bool pending=welcomePanoramaCursor<welcomePanoramaEntries.size();bool moving=welcomeMotionAllowed()&&!pending&&!welcomePanorama.empty();
 advanceWelcomePanorama(panoramaNowSeconds(),moving);
 if(cycle>0){float x=-float(panoramaOffsetFor(welcomePanoramaSeconds,cycle));size_t index=0;while(x<width&&index<1024){auto& bitmap=welcomePanorama[index%welcomePanorama.size()];auto size=bitmap->GetSize();float w=height*size.width/size.height;if(x+w>0)renderer.target->DrawBitmap(bitmap.Get(),D2D1::RectF(x,0,x+w,height),1,D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);x+=w;index++;}}
 updateWelcomeTimer(pending||moving);
}
HWND navTooltip=nullptr;std::vector<std::wstring> navHintTexts;
std::vector<std::pair<int,std::wstring>> navigationPages()const{
 std::vector<std::pair<int,std::wstring>> pages={{0,L"이미지 생성"},{1,L"참조 이미지"},{2,L"크기·품질"},{5,L"이미지 보정·확대"},{16,L"후보 만들기·비교"},{17,L"페이지·컷 구성"},{18,L"좋은 시드 추천"},{3,L"진행 기록"},{9,L"서버 이미지"}};
 if(modelsSupported())pages.push_back({14,L"모델 추가·관리"});
 if(developer()){pages.push_back({7,L"캐릭터·화풍 일관성"});pages.push_back({8,L"내 이미지로 학습"});pages.push_back({15,L"여러 각도 참조 시트"});pages.push_back({11,L"서버 관리"});}
 pages.push_back({4,L"세부 설정"});return pages;
}
float navigationStep()const{return std::clamp((height-324)/std::max(1.f,float(navigationPages().size()-1)),24.f,36.f);}
float navigationTileHeight()const{return std::min(34.f,navigationStep()-2);}
void openNavigation(int id){if(id==16)auditionMenu();else if(id==17)storyboardMenu();else if(id==18)friendlySeedMenu();else if(id==4)openDetails(1);else{setPage(id);if(id==11&&!developerBusy)developerFetch();}}
int navigationGlyph(int id)const{switch(id){case 0:return 0;case 1:return 1;case 2:return 2;case 3:return 9;case 4:return 14;case 5:return 7;case 7:return 10;case 8:return 8;case 9:return 1;case 11:return 4;case 14:return 15;case 15:return 6;case 16:return 6;case 17:return 3;case 18:return 0;default:return 10;}}
void navigationPaint(){
 renderer.box({8,8,48,height-16},0x19191a,0x303033,10);
 auto tile=[&](Rect r,int glyph,bool active,std::function<void()> command){renderer.box(r,active?0x34343b:0x19191a,0,8);icon({r.x+(r.w-16)/2,r.y+(r.h-16)/2,16,16},glyph,active?0xb5c2ff:0x999eaf);buttons.push_back({r,L"",command,false});};
 tile({15,20,34,34},11,projectsOpen,[this]{save();projectsOpen=!projectsOpen;leftWidth=projectsOpen?260.f:64.f;autoWorkspace=true;rebuild(false);});
 int row=0;for(auto& [id,name]:navigationPages()){tile({15,76+row*navigationStep(),34,navigationTileHeight()},navigationGlyph(id),id==page,[this,id]{openNavigation(id);});row++;}
 float y=height-166;tile({15,y,34,34},12,false,[this]{save();status=L"저장 완료";});tile({15,y+40,34,34},11,false,[this]{projectMenu();});tile({15,y+80,34,34},14,page==10,[this]{setPage(10);});tile({15,y+120,34,34},5,page==13,[this]{accountMenu();});
}
void updateNavigationTips(){
 auto pages=navigationPages();std::vector<std::wstring> texts{L"프로젝트 목록"};for(auto& p:pages)texts.push_back(p.second);texts.push_back(L"프로젝트 저장");texts.push_back(L"프로젝트 관리");texts.push_back(L"전체 설정");texts.push_back(L"내 계정");
 auto rectangle=[&](size_t i){float y=i==0?20.f:i<=pages.size()?76+(i-1)*navigationStep():height-166+(i-pages.size()-1)*40.f;float h=i>0&&i<=pages.size()?navigationTileHeight():34.f;return RECT{LONG(15*dpi),LONG(y*dpi),LONG(49*dpi),LONG((y+h)*dpi)};};
 if(navTooltip&&!welcome&&texts==navHintTexts){for(size_t i=0;i<navHintTexts.size();i++){TOOLINFOW tool{sizeof tool};tool.hwnd=hwnd;tool.uId=i+1;tool.rect=rectangle(i);SendMessageW(navTooltip,TTM_NEWTOOLRECTW,0,(LPARAM)&tool);}return;}
 if(navTooltip)DestroyWindow(navTooltip);navTooltip=nullptr;navHintTexts.clear();if(!hwnd||welcome)return;navHintTexts=std::move(texts);
 navTooltip=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,hwnd,nullptr,GetModuleHandle(nullptr),nullptr);
 SendMessageW(navTooltip,TTM_SETDELAYTIME,TTDT_INITIAL,350);SendMessageW(navTooltip,TTM_SETMAXTIPWIDTH,0,240);SetWindowTheme(navTooltip,L"DarkMode_Explorer",nullptr);
 for(size_t i=0;i<navHintTexts.size();i++){TOOLINFOW tool{sizeof tool};tool.uFlags=TTF_SUBCLASS;tool.hwnd=hwnd;tool.uId=i+1;tool.rect=rectangle(i);tool.lpszText=navHintTexts[i].data();SendMessageW(navTooltip,TTM_ADDTOOLW,0,(LPARAM)&tool);}
}
void paintWelcome(){
 browser={};previewRect={};accountPopover=false;
 paintWelcomePanorama();
 if(!welcomeLogo)welcomeLogo=renderer.resource(203);
 renderer.fill({0,0,width,height},0x101011,welcomeBackground?.72f:1.f);
 float w=std::min(520.f,width-80),x=(width-w)/2,y=std::max(48.f,(height-430)/2);
 renderer.box({x,y,w,430},0x1b1b1c,0x333338,18);renderer.bitmap(welcomeLogo.Get(),{x+w/2-26,y+30,52,52});
 renderer.text(L"SNUWCANVAS",{x+40,y+101,w-80,45},0xe9e8f2,28,true,true);
 renderer.text(L"이미지로 시작하는 새로운 작업",{x+40,y+157,w-80,30},0xb2b1c7,15,false,true);
 button({x+40,y+242,w-80,42},L"로그인",[this]{accountSignIn();},true);
 button({x+40,y+296,w-80,42},L"계정 만들기",[this]{accountSignIn(true);});
 paintStudioBrand({x+40,y+389,w-80,24});
 if constexpr(customServerEdition)button({x+40,y+350,w-80,32},L"서버 주소 변경",[this]{configureCustomServer();});

}
