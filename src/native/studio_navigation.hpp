bool welcome=false,projectsOpen=false;
HWND navTooltip=nullptr;std::vector<std::wstring> navHintTexts;
std::vector<std::pair<int,std::wstring>> navigationPages()const{
 std::vector<std::pair<int,std::wstring>> out;
 const std::vector<std::wstring> names={L"이미지 생성",L"참조 이미지",L"생성 설정",L"진행 로그",L"고급 설정",L"이미지 보정",L"자연어 편집",L"일관성",L"학습",L"서버 이미지"};
 for(int i=0;i<10;i++)if((i!=7&&i!=8)||developer())out.push_back({i,names[i]});return out;
}
void navigationPaint(){
 renderer.box({8,8,48,height-16},0x19191a,0x303033,10);
 auto tile=[&](Rect r,int glyph,bool active,std::function<void()> command){renderer.box(r,active?0x34343b:0x19191a,0,8);icon({r.x+9,r.y+9,16,16},glyph,active?0xb5c2ff:0x999eaf);buttons.push_back({r,L"",command,false});};
 tile({15,20,34,34},3,projectsOpen,[this]{save();projectsOpen=!projectsOpen;leftWidth=projectsOpen?260.f:64.f;autoWorkspace=true;rebuild(false);});
 const int glyphs[]={0,1,2,3,4,6,7,10,8,9};int row=0;for(auto& [id,name]:navigationPages()){tile({15,76+row*42.f,34,34},glyphs[id],id==page,[this,id]{setPage(id);});row++;}
 tile({15,height-98,34,34},2,page==10,[this]{setPage(10);});tile({15,height-52,34,34},5,false,[this]{accountMenu();});
}
void updateNavigationTips(){
 if(navTooltip)DestroyWindow(navTooltip);navTooltip=nullptr;navHintTexts.clear();if(!hwnd||welcome)return;
 auto pages=navigationPages();navHintTexts.reserve(14);navHintTexts.push_back(L"프로젝트 목록");for(auto& p:pages)navHintTexts.push_back(p.second);navHintTexts.push_back(L"전체 설정");navHintTexts.push_back(L"내 계정");
 navTooltip=CreateWindowExW(WS_EX_TOPMOST,TOOLTIPS_CLASSW,nullptr,WS_POPUP|TTS_ALWAYSTIP|TTS_NOPREFIX,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,hwnd,nullptr,GetModuleHandle(nullptr),nullptr);
 SendMessageW(navTooltip,TTM_SETDELAYTIME,TTDT_INITIAL,350);SendMessageW(navTooltip,TTM_SETMAXTIPWIDTH,0,240);SetWindowTheme(navTooltip,L"DarkMode_Explorer",nullptr);
 for(size_t i=0;i<navHintTexts.size();i++){float y=i==0?20.f:i==navHintTexts.size()-1?height-52:i==navHintTexts.size()-2?height-98:76+(i-1)*42.f;TOOLINFOW tool{sizeof tool};tool.uFlags=TTF_SUBCLASS;tool.hwnd=hwnd;tool.uId=i+1;tool.rect={LONG(15*dpi),LONG(y*dpi),LONG(49*dpi),LONG((y+34)*dpi)};tool.lpszText=navHintTexts[i].data();SendMessageW(navTooltip,TTM_ADDTOOLW,0,(LPARAM)&tool);}
}
void paintWelcome(){
 browser={};previewRect={};accountPopover=false;
 float w=std::min(520.f,width-80),x=(width-w)/2,y=std::max(48.f,(height-430)/2);
 renderer.box({x,y,w,430},0x1b1b1c,0x333338,18);icon({x+w/2-18,y+34,36,36},0,0xb5c2ff);
 label(L"SNUWCANVAS",{x+40,y+89,w-80,45},true,0xe9e8f2,28);
 label(L"이미지로 시작하는 새로운 작업",{x+40,y+145,w-80,30},false,0xb2b1c7,15);
 label(L"로그인 후 이미지 생성 서비스를 이용하세요.",{x+40,y+187,w-80,28},false,0x9695ad,12);
 button({x+40,y+242,w-80,42},L"로그인",[this]{accountSignIn();},true);
 button({x+40,y+296,w-80,42},L"계정 만들기",[this]{accountSignIn(true);});

}
