bool welcome=false,projectsOpen=false;
Com<ID2D1Bitmap> welcomeBackground,welcomeLogo;
bool welcomeBackgroundChecked=false;
void loadApprovedWelcomeBackground(){
 if(welcomeBackgroundChecked)return;welcomeBackgroundChecked=true;
 try{
  auto bytes=[](int key,DWORD limit){auto resource=FindResourceW(nullptr,MAKEINTRESOURCEW(key),RT_RCDATA);if(!resource)return std::string{};auto size=SizeofResource(nullptr,resource);if(!size||size>limit)return std::string{};auto data=LockResource(LoadResource(nullptr,resource));return data?std::string(static_cast<const char*>(data),size):std::string{};};
  auto manifest=json::parse(bytes(205,65536));if(!manifest.is_object()||manifest.value("schema",0)!=1||str(manifest,"motion")!="none"||!manifest.contains("entries")||!manifest["entries"].is_array()||manifest["entries"].size()>7)return;
  for(const auto& entry:manifest["entries"]){
   if(!entry.is_object()||!flag(entry,"safe_approved")||!flag(entry,"rights_approved")||str(entry,"source").empty()||str(entry,"license").empty()||str(entry,"reviewed_by").empty()||str(entry,"reviewed_at").empty())continue;
   // Only the bundled image slot is supported; manifests cannot read local paths or URLs.
   if(!entry.contains("resource")||!entry["resource"].is_number_integer()||entry["resource"]!=202)continue;
   auto expected=str(entry,"sha256");if(expected.size()!=64||expected.find_first_not_of("0123456789abcdef")!=std::string::npos)continue;
   auto image=bytes(202,25*1024*1024);if(image.empty()||sha256(image)!=expected)continue;
   auto bitmap=renderer.resource(202);if(!bitmap)continue;auto size=bitmap->GetSize();if(size.width<1||size.height<1||size.width>16384||size.height>16384)continue;
   welcomeBackground=bitmap;return;
  }
 }catch(...){welcomeBackground.Reset();}
}
HWND navTooltip=nullptr;std::vector<std::wstring> navHintTexts;
std::vector<std::pair<int,std::wstring>> navigationPages()const{
 std::vector<std::pair<int,std::wstring>> out;
 const std::vector<std::wstring> names={L"이미지 생성",L"참조 이미지",L"생성 설정",L"진행 로그",L"고급 설정",L"이미지 보정",L"자연어 편집",L"일관성",L"학습",L"서버 이미지"};
 for(int i=0;i<10;i++)if(i!=6&&((i!=7&&i!=8)||developer()))out.push_back({i,names[i]});if(developer()){out.push_back({15,L"참조 시트"});out.push_back({11,L"개발자 관리"});}if(modelsSupported())out.push_back({14,L"모델 관리"});return out;
}
float navigationStep()const{return std::clamp((height-324)/std::max(1.f,float(navigationPages().size()-1)),24.f,36.f);}
void navigationPaint(){
 renderer.box({8,8,48,height-16},0x19191a,0x303033,10);
 auto tile=[&](Rect r,int glyph,bool active,std::function<void()> command){renderer.box(r,active?0x34343b:0x19191a,0,8);icon({r.x+9,r.y+9,16,16},glyph,active?0xb5c2ff:0x999eaf);buttons.push_back({r,L"",command,false});};
 tile({15,20,34,34},11,projectsOpen,[this]{save();projectsOpen=!projectsOpen;leftWidth=projectsOpen?260.f:64.f;autoWorkspace=true;rebuild(false);});
 const int glyphs[]={0,1,2,9,4,6,7,10,8,15};int row=0;for(auto& [id,name]:navigationPages()){tile({15,76+row*navigationStep(),34,34},id==15?10:id==14?15:id==11?4:glyphs[id],id==page,[this,id]{setPage(id);if(id==11&&!developerBusy)developerFetch();});row++;}
 float y=height-166;tile({15,y,34,34},12,false,[this]{save();status=L"저장 완료";});tile({15,y+40,34,34},11,false,[this]{projectMenu();});tile({15,y+80,34,34},14,page==10,[this]{setPage(10);});tile({15,y+120,34,34},5,page==13,[this]{accountMenu();});
}
void updateNavigationTips(){
 if(navTooltip&&!welcome){auto pages=navigationPages();for(size_t i=0;i<navHintTexts.size();i++){float y=i==0?20.f:i<=pages.size()?76+(i-1)*navigationStep():height-166+(i-pages.size()-1)*40.f;TOOLINFOW tool{sizeof tool};tool.hwnd=hwnd;tool.uId=i+1;tool.rect={LONG(15*dpi),LONG(y*dpi),LONG(49*dpi),LONG((y+34)*dpi)};SendMessageW(navTooltip,TTM_NEWTOOLRECTW,0,(LPARAM)&tool);}return;}
 if(navTooltip)DestroyWindow(navTooltip);navTooltip=nullptr;navHintTexts.clear();if(!hwnd||welcome)return;
 auto pages=navigationPages();navHintTexts.reserve(20);navHintTexts.push_back(L"프로젝트 목록");for(auto& p:pages)navHintTexts.push_back(p.second);navHintTexts.push_back(L"프로젝트 저장");navHintTexts.push_back(L"프로젝트 관리");navHintTexts.push_back(L"전체 설정");navHintTexts.push_back(L"내 계정");
 navTooltip=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,hwnd,nullptr,GetModuleHandle(nullptr),nullptr);
 SendMessageW(navTooltip,TTM_SETDELAYTIME,TTDT_INITIAL,350);SendMessageW(navTooltip,TTM_SETMAXTIPWIDTH,0,240);SetWindowTheme(navTooltip,L"DarkMode_Explorer",nullptr);
 for(size_t i=0;i<navHintTexts.size();i++){float y=i==0?20.f:i<=pages.size()?76+(i-1)*navigationStep():height-166+(i-pages.size()-1)*40.f;TOOLINFOW tool{sizeof tool};tool.uFlags=TTF_SUBCLASS;tool.hwnd=hwnd;tool.uId=i+1;tool.rect={LONG(15*dpi),LONG(y*dpi),LONG(49*dpi),LONG((y+34)*dpi)};tool.lpszText=navHintTexts[i].data();SendMessageW(navTooltip,TTM_ADDTOOLW,0,(LPARAM)&tool);}
}
void paintWelcome(){
 browser={};previewRect={};accountPopover=false;
 if(!welcomeBackground)loadApprovedWelcomeBackground();
 if(!welcomeLogo)welcomeLogo=renderer.resource(203);
 if(welcomeBackground){auto size=welcomeBackground->GetSize();float scale=std::max(width/size.width,height/size.height);float sw=width/scale,sh=height/scale;renderer.target->DrawBitmap(welcomeBackground.Get(),D2D1::RectF(0,0,width,height),1,D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,D2D1::RectF((size.width-sw)/2,(size.height-sh)/2,(size.width+sw)/2,(size.height+sh)/2));}
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
