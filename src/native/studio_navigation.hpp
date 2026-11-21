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
void openNavigation(int id){if(id==18)friendlySeedMenu();else if(id==4)openSettings(2);else{setPage(id);if(id==11&&!developerBusy)developerFetch();}}
// Rail glyphs: one distinct picture per feature so the rail reads without tooltips.
void featureIcon(Rect r,int kind,unsigned tint){
 float x=r.x,y=r.y,w=r.w,h=r.h;
 auto line=[&](float a,float b,float c,float d){renderer.line(x+a*w,y+b*h,x+c*w,y+d*h,tint,1.4f);};
 auto path=[&](std::initializer_list<float> v,bool closed){std::vector<float> p(v);for(size_t i=0;i+3<p.size();i+=2)line(p[i],p[i+1],p[i+2],p[i+3]);if(closed&&p.size()>=4)line(p[p.size()-2],p[p.size()-1],p[0],p[1]);};
 auto box=[&](float a,float b,float c,float d){path({a,b,c,b,c,d,a,d},true);};
 auto circle=[&](float cx,float cy,float radius){renderer.brush->SetColor(color(tint));renderer.target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x+w*cx,y+h*cy),w*radius,h*radius),renderer.brush.Get(),1.4f);};
 switch(kind){
  case 20:path({.45f,.13f,.55f,.45f,.87f,.55f,.55f,.65f,.45f,.97f,.35f,.65f,.03f,.55f,.35f,.45f},true);line(.84f,.03f,.84f,.27f);line(.72f,.15f,.96f,.15f);break; // generate: sparkle
  case 21:box(.06f,.14f,.94f,.86f);circle(.68f,.36f,.09f);path({.06f,.8f,.36f,.48f,.58f,.72f,.7f,.6f,.94f,.82f},false);break; // reference picture
  case 22:box(.08f,.5f,.5f,.92f);line(.4f,.6f,.9f,.1f);line(.9f,.1f,.62f,.1f);line(.9f,.1f,.9f,.38f);break; // size: resize arrow
  case 23:circle(.42f,.42f,.32f);line(.65f,.65f,.94f,.94f);line(.42f,.27f,.42f,.57f);line(.27f,.42f,.57f,.42f);break; // enhance: zoom in
  case 24:box(.04f,.2f,.46f,.8f);box(.54f,.2f,.96f,.8f);path({.62f,.52f,.72f,.63f,.88f,.4f},false);break; // compare candidates
  case 25:box(.06f,.1f,.94f,.9f);line(.06f,.5f,.94f,.5f);line(.42f,.1f,.42f,.5f);line(.62f,.5f,.62f,.9f);break; // comic panels
  case 26:line(.5f,.95f,.5f,.45f);path({.5f,.64f,.2f,.52f,.1f,.28f,.38f,.36f},true);path({.5f,.5f,.78f,.38f,.92f,.12f,.62f,.18f},true);line(.25f,.95f,.75f,.95f);break; // seed: sprout
  case 27:path({.22f,.24f,.22f,.08f,.94f,.08f,.94f,.66f,.78f,.66f},false);box(.06f,.24f,.78f,.92f);path({.06f,.86f,.3f,.6f,.48f,.78f,.6f,.68f,.78f,.86f},false);break; // server gallery
  case 28:path({.5f,.05f,.92f,.28f,.92f,.72f,.5f,.95f,.08f,.72f,.08f,.28f},true);path({.08f,.28f,.5f,.5f,.92f,.28f},false);line(.5f,.5f,.5f,.95f);break; // model: cube
  case 29:circle(.38f,.34f,.15f);path({.1f,.92f,.12f,.74f,.25f,.62f,.51f,.62f,.64f,.74f,.66f,.92f},false);circle(.72f,.26f,.12f);path({.68f,.5f,.84f,.5f,.94f,.62f,.94f,.8f},false);break; // consistency: same character
  case 30:path({.5f,.15f,.95f,.35f,.5f,.55f,.05f,.35f},true);path({.25f,.45f,.25f,.72f,.5f,.84f,.75f,.72f,.75f,.45f},false);line(.95f,.35f,.95f,.66f);break; // training: cap
  case 31:{circle(.5f,.26f,.13f);path({.36f,.66f,.36f,.48f,.64f,.48f,.64f,.66f},false);float px=0,py=0;for(int i=0;i<=12;i++){float a=(-150.f+i*25.f)*3.14159265f/180;float qx=.5f+.44f*std::cos(a),qy=.72f+.2f*std::sin(a);if(i)line(px,py,qx,qy);px=qx;py=qy;}line(px,py,px+.02f,py-.16f);line(px,py,px+.16f,py+.04f);break;} // angles: orbit
  case 32:box(.06f,.12f,.94f,.88f);line(.36f,.12f,.36f,.88f);line(.14f,.32f,.28f,.32f);line(.14f,.48f,.28f,.48f);line(.14f,.64f,.28f,.64f);break; // project list panel
  case 33:line(.5f,.06f,.5f,.62f);path({.28f,.42f,.5f,.64f,.72f,.42f},false);path({.1f,.66f,.1f,.92f,.9f,.92f,.9f,.66f},false);break; // save: download into tray
  case 34:path({.06f,.84f,.06f,.2f,.36f,.2f,.5f,.34f,.94f,.34f,.94f,.84f},true);break; // open folder
  case 35:circle(.18f,.5f,.07f);circle(.5f,.5f,.07f);circle(.82f,.5f,.07f);break; // more: three dots
  case 36:line(.9f,.5f,.14f,.5f);path({.42f,.2f,.14f,.5f,.42f,.8f},false);break; // previous
  case 37:line(.1f,.5f,.86f,.5f);path({.58f,.2f,.86f,.5f,.58f,.8f},false);break; // next
  case 38:circle(.5f,.5f,.42f);line(.08f,.5f,.92f,.5f);path({.5f,.08f,.34f,.3f,.3f,.5f,.34f,.7f,.5f,.92f},false);path({.5f,.08f,.66f,.3f,.7f,.5f,.66f,.7f,.5f,.92f},false);break; // url: globe
  case 39:line(.12f,.25f,.88f,.25f);path({.4f,.25f,.4f,.1f,.6f,.1f,.6f,.25f},false);path({.22f,.25f,.28f,.92f,.72f,.92f,.78f,.25f},false);line(.42f,.42f,.43f,.78f);line(.58f,.42f,.57f,.78f);break; // delete: trash
  case 40:circle(.5f,.5f,.2f);circle(.5f,.5f,.34f);line(.5f,.06f,.5f,.16f);line(.5f,.84f,.5f,.94f);line(.06f,.5f,.16f,.5f);line(.84f,.5f,.94f,.5f);line(.19f,.19f,.26f,.26f);line(.81f,.19f,.74f,.26f);line(.19f,.81f,.26f,.74f);line(.81f,.81f,.74f,.74f);break; // general: gear
  case 41:path({.12f,.88f,.52f,.48f},false);path({.52f,.48f,.68f,.32f,.9f,.1f},false);line(.3f,.1f,.3f,.3f);line(.2f,.2f,.4f,.2f);circle(.8f,.76f,.06f);break; // generation: brush + sparkle
  case 42:path({.5f,.08f,.88f,.28f,.88f,.72f,.5f,.92f,.12f,.72f,.12f,.28f},true);path({.12f,.28f,.5f,.48f,.88f,.28f},false);line(.5f,.48f,.5f,.92f);break; // model: cube
  case 43:circle(.36f,.32f,.15f);path({.08f,.9f,.1f,.64f,.36f,.56f,.62f,.64f,.64f,.9f},false);line(.82f,.18f,.82f,.42f);line(.7f,.3f,.94f,.3f);break; // reference: person + add
  case 44:path({.06f,.84f,.06f,.2f,.36f,.2f,.5f,.34f,.94f,.34f,.94f,.84f},true);line(.5f,.46f,.5f,.72f);path({.4f,.62f,.5f,.72f,.6f,.62f},false);break; // storage: folder + arrow
  case 45:path({.1f,.86f,.3f,.82f,.44f,.54f,.58f,.22f,.9f,.14f},false);circle(.1f,.86f,.05f);circle(.9f,.14f,.05f);break; // motion: easing curve
  case 46:line(.36f,.08f,.36f,.34f);line(.64f,.08f,.64f,.34f);path({.22f,.34f,.78f,.34f,.78f,.54f,.5f,.72f,.22f,.54f},true);line(.5f,.72f,.5f,.92f);break; // connection: plug
  case 47:circle(.42f,.42f,.28f);line(.63f,.63f,.92f,.92f);break; // search
  case 48:path({.84f,.5f,.8f,.3f,.64f,.16f,.44f,.14f,.26f,.26f,.16f,.44f,.2f,.66f,.36f,.82f,.58f,.86f,.78f,.76f},false);path({.92f,.26f,.84f,.5f,.62f,.4f},false);break; // reset: circular arrow
  case 49:path({.5f,.1f,.92f,.86f,.08f,.86f},true);line(.5f,.38f,.5f,.62f);circle(.5f,.74f,.03f);break; // warning
  case 50:line(.2f,.2f,.8f,.8f);line(.8f,.2f,.2f,.8f);break; // close
 }
}
int navigationGlyph(int id)const{switch(id){case 0:return 20;case 1:return 21;case 2:return 22;case 3:return 9;case 4:return 2;case 5:return 23;case 7:return 29;case 8:return 30;case 9:return 27;case 11:return 15;case 14:return 28;case 15:return 31;case 16:return 24;case 17:return 25;case 18:return 26;default:return 10;}}
void navigationPaint(){
 renderer.box({8,8,48,height-16},0x19191a,0x303033,10);
 auto tile=[&](Rect r,int glyph,bool active,std::function<void()> command){renderer.box(r,active?0x34343b:0x19191a,0,8);icon({r.x+(r.w-16)/2,r.y+(r.h-16)/2,16,16},glyph,active?0xb5c2ff:0x999eaf);buttons.push_back({r,L"",command,false});};
 tile({15,20,34,34},32,projectsOpen,[this]{toggleProjectPane();});
 int row=0;for(auto& [id,name]:navigationPages()){tile({15,76+row*navigationStep(),34,navigationTileHeight()},navigationGlyph(id),id==page,[this,id]{openNavigation(id);});row++;}
 float y=height-166;tile({15,y,34,34},12,false,[this]{save();status=L"저장 완료";});tile({15,y+40,34,34},11,false,[this]{projectMenu();});tile({15,y+80,34,34},14,page==10||settingsOpen,[this]{openSettings(0);});tile({15,y+120,34,34},5,page==13,[this]{accountMenu();});
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
