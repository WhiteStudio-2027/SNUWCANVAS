// 통합 설정 패널 (모달 오버레이). App 클래스 본문 안에서 include 된다.
bool settingsOpen=false;int settingsCat=0;float settingsScroll=0,settingsScrollTarget=0,settingsContent=0,settingsViewH=0;
std::wstring settingsQuery,settingsNotice;ULONGLONG settingsNoticeUntil=0;HWND settingsSearch=nullptr,settingsEdit=nullptr;
std::string settingsEditKey;Rect settingsWin{0,0,0,0};std::vector<HWND> settingsHidden;json settingsBase=json::object();
struct SettingsHit{Rect r,clip;std::function<void()> f;};std::vector<SettingsHit> settingsHits;
bool motionOn=true;ULONGLONG motionChecked=0;
bool motionEnabled(){ULONGLONG n=GetTickCount64();if(!motionChecked||n-motionChecked>1000){motionChecked=n;auto mode=str(store->settings,"motion_mode","system");if(mode=="on")motionOn=true;else if(mode=="off")motionOn=false;else{BOOL enabled=TRUE;SystemParametersInfoW(0x1042,0,&enabled,0);motionOn=enabled!=FALSE;}}return motionOn;}
static unsigned long long sKey(const std::string& k,int salt){return (unsigned long long)std::hash<std::string>{}(k)*131u+(unsigned long long)salt+0x9e3779b97f4a7c15ull;}
static float sTextW(const std::wstring& s,float size){float w=0;for(wchar_t c:s)w+=c>127?size:size*.58f;return w+4;}
static std::wstring sFit(const std::wstring& s,float size,float width){if(sTextW(s,size)<=width)return s;std::wstring out;float w=sTextW(L"…",size);for(wchar_t c:s){float cw=c>127?size:size*.58f;if(w+cw>width)break;out.push_back(c);w+=cw;}return out+L"…";}
void sHit(Rect r,Rect clip,std::function<void()> f){settingsHits.push_back({r,clip,std::move(f)});}
bool sHover(Rect r,Rect clip){return r.hit(mouseX,mouseY)&&clip.hit(mouseX,mouseY);}
void settingsToast(const std::wstring& text){settingsNotice=text;settingsNoticeUntil=GetTickCount64()+2400;animPending=true;if(hwnd)InvalidateRect(hwnd,nullptr,FALSE);}
json settingDefault(const std::string& key){auto ud=store->settings.value("_user_defaults",json::object());if(ud.is_object()&&ud.contains(key))return ud[key];return settingsBase.contains(key)?settingsBase[key]:json();}
bool settingResettable(const SettingDef& d){return d.resettable&&d.type!=SettingType::Action&&d.type!=SettingType::Info&&d.key!="output";}
bool settingModified(const SettingDef& d){if(!settingResettable(d))return false;auto def=settingDefault(d.key);auto cur=store->settings.contains(d.key)?store->settings[d.key]:json();if(d.type==SettingType::Toggle)return flag(store->settings,d.key)!=(def.is_boolean()?def.get<bool>():false);auto a=settingText(cur),b=settingText(def);if(a==b)return false;if(d.type==SettingType::Number){try{size_t e1=0,e2=0;double x=std::stod(a,&e1),y=std::stod(b,&e2);if(e1==a.size()&&e2==b.size()&&x==y)return false;}catch(...){}}return true;}
void setSettingValue(const std::string& key,const json& value){store->settings[key]=value;}
void commitSettingValues(){try{store->save();}catch(const std::exception&){settingsToast(L"설정을 저장하지 못했습니다.");}if(hwnd)InvalidateRect(hwnd,nullptr,FALSE);}
void applySetting(const std::string& key,const json& value){setSettingValue(key,value);commitSettingValues();}
void pushSettingsToFields(){std::set<std::string> keys;for(auto& d:settingRegistry())keys.insert(d.key);for(auto& f:fields){if(f.key.empty()||!keys.count(f.key)||!f.hwnd||!IsWindow(f.hwnd)||!store->settings.contains(f.key))continue;if(f.checkbox){SendMessageW(f.hwnd,BM_SETCHECK,flag(store->settings,f.key)?BST_CHECKED:BST_UNCHECKED,0);continue;}auto value=wide(str(store->settings,f.key));wchar_t cls[32]{};GetClassNameW(f.hwnd,cls,32);if(std::wstring(cls)==L"ComboBox"&&(GetWindowLongPtrW(f.hwnd,GWL_STYLE)&3)==CBS_DROPDOWNLIST){auto i=SendMessageW(f.hwnd,CB_FINDSTRINGEXACT,-1,(LPARAM)value.c_str());if(i==CB_ERR)i=SendMessageW(f.hwnd,CB_ADDSTRING,0,(LPARAM)value.c_str());SendMessageW(f.hwnd,CB_SETCURSEL,i,0);}else SetWindowTextW(f.hwnd,value.c_str());}}
void hideForSettings(){for(HWND c=GetWindow(hwnd,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(c==settingsSearch||c==settingsEdit)continue;if(IsWindowVisible(c)){ShowWindow(c,SW_HIDE);if(std::find(settingsHidden.begin(),settingsHidden.end(),c)==settingsHidden.end())settingsHidden.push_back(c);}}}
void cancelSettingsEdit(){if(!settingsEdit)return;HWND h=settingsEdit;settingsEdit=nullptr;settingsEditKey.clear();DestroyWindow(h);if(hwnd)InvalidateRect(hwnd,nullptr,FALSE);}
void commitSettingsEdit(){if(!settingsEdit)return;HWND h=settingsEdit;auto key=settingsEditKey;auto text=textOf(h);settingsEdit=nullptr;settingsEditKey.clear();DestroyWindow(h);size_t a=text.find_first_not_of(L" \t\r\n"),b=text.find_last_not_of(L" \t\r\n");text=a==std::wstring::npos?L"":text.substr(a,b-a+1);const SettingDef* def=nullptr;for(auto& d:settingRegistry())if(d.key==key)def=&d;if(!def)return;
 if(def->type==SettingType::Number){size_t end=0;double n=0;try{n=std::stod(text,&end);}catch(...){end=0;}if(text.empty()||end!=text.size()||!std::isfinite(n)||n<0||(def->integer&&text.find_first_of(L".eE")!=std::wstring::npos)){settingsToast(def->integer?L"0 이상의 정수를 입력하세요.":L"0 이상의 숫자를 입력하세요.");return;}}
 else if(key=="node_overrides"){if(text.empty())text=L"{}";if(json::parse(utf8(text),nullptr,false).is_discarded()){settingsToast(L"올바른 JSON 형식이 아닙니다.");return;}}
 else if(text.empty()){settingsToast(L"빈 값은 저장하지 않습니다.");return;}
 if(utf8(text)!=settingText(store->settings.value(key,json())))applySetting(key,utf8(text));else if(hwnd)InvalidateRect(hwnd,nullptr,FALSE);}
void beginSettingsEdit(const SettingDef& d,Rect r){commitSettingsEdit();settingsEditKey=d.key;settingsEdit=control(L"EDIT",wide(settingText(store->settings.value(d.key,json()))),ES_AUTOHSCROLL,1901);place(settingsEdit,{r.x+9,r.y+6,r.w-18,r.h-12});SetFocus(settingsEdit);SendMessageW(settingsEdit,EM_SETSEL,0,-1);childrenStale=true;}
void openSettings(int cat=-1){if(settingsOpen){if(cat>=0){commitSettingsEdit();settingsCat=std::clamp(cat,0,int(settingCategories().size())-1);settingsScrollTarget=0;}return;}if(binding||promptComposing||!hwnd)return;settingsOpen=true;if(cat>=0)settingsCat=std::clamp(cat,0,int(settingCategories().size())-1);settingsQuery.clear();settingsScroll=settingsScrollTarget=0;settingsBase=defaults(store->root);anim[animKey({0,0,3,3},31)]=0;settingsSearch=control(L"EDIT",L"",ES_AUTOHSCROLL,1900);SendMessageW(settingsSearch,EM_SETCUEBANNER,TRUE,(LPARAM)L"설정 검색");hideForSettings();SetFocus(settingsSearch);InvalidateRect(hwnd,nullptr,FALSE);}
void closeSettings(){if(!settingsOpen)return;commitSettingsEdit();if(settingsSearch){HWND h=settingsSearch;settingsSearch=nullptr;DestroyWindow(h);}settingsOpen=false;pushSettingsToFields();for(HWND c:settingsHidden)if(IsWindow(c))ShowWindow(c,SW_SHOWNA);settingsHidden.clear();settingsQuery.clear();childrenStale=true;SetFocus(hwnd);RedrawWindow(hwnd,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);}
void toggleSettings(){if(settingsOpen)closeSettings();else openSettings();}
void settingsKey(WPARAM key){if(key==VK_ESCAPE){if(settingsEdit)cancelSettingsEdit();else if(!settingsQuery.empty()&&settingsSearch){SetWindowTextW(settingsSearch,L"");}else closeSettings();}else if(key==VK_RETURN)commitSettingsEdit();}
void settingsSearchChanged(){if(!settingsSearch)return;settingsQuery=textOf(settingsSearch);settingsScroll=settingsScrollTarget=0;if(hwnd)InvalidateRect(hwnd,nullptr,FALSE);}
void settingsWheel(int delta){commitSettingsEdit();float limit=std::max(0.f,settingsContent-settingsViewH);settingsScrollTarget=std::clamp(settingsScrollTarget-float(delta)/120.f*70.f,0.f,limit);if(hwnd)InvalidateRect(hwnd,nullptr,FALSE);}
void settingsDown(float x,float y){SetFocus(hwnd);commitSettingsEdit();for(auto it=settingsHits.rbegin();it!=settingsHits.rend();++it)if(it->r.hit(x,y)&&it->clip.hit(x,y)){auto f=it->f;f();if(hwnd)InvalidateRect(hwnd,nullptr,FALSE);return;}if(!settingsWin.hit(x,y))closeSettings();}
void settingsAction(const std::string& key){
 if(key=="content_mode"){closeSettings();try{toggleContentMode();}catch(const std::exception& e){status=wide(userError(e.what()));}}
 else if(key=="server"){closeSettings();try{if constexpr(customServerEdition)configureCustomServer();else changeServerConnection();}catch(const std::exception& e){status=wide(userError(e.what()));}}
 else if(key=="reset"){if(MessageBoxW(hwnd,L"모든 설정을 처음 상태로 되돌릴까요?\n저장해 둔 기본값도 함께 지워집니다.",L"설정 초기화",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)==IDYES){store->resetSettings();store->settings["_user_defaults"]=json::object();settingsBase=defaults(store->root);commitSettingValues();rebuild(false);settingsToast(L"설정을 처음 상태로 되돌렸습니다.");}}}
void settingsSaveDefaults(int cat){auto ud=store->settings.value("_user_defaults",json::object());if(!ud.is_object())ud=json::object();for(auto& d:settingRegistry())if(d.category==cat&&settingResettable(d)&&store->settings.contains(d.key))ud[d.key]=store->settings[d.key];applySetting("_user_defaults",ud);settingsToast(L"현재 값을 기본값으로 저장했습니다.");}
void settingsRestoreDefaults(int cat){for(auto& d:settingRegistry())if(d.category==cat&&settingResettable(d)){auto v=settingDefault(d.key);if(!v.is_null())setSettingValue(d.key,v);}commitSettingValues();settingsToast(L"기본값으로 되돌렸습니다.");}
void sButton(Rect r,Rect clip,const std::string& key,const std::wstring& label,unsigned ink,std::function<void()> f){bool over=sHover(r,clip);float t=animate(sKey(key,6),over?1.f:0.f,90.f);renderer.box(r,mixColor(0x2a2a2f,0x3a3a42,t),mixColor(0x3b3b40,0x6a6a78,t),7);renderer.text(label,r,ink,12,false,true);sHit(r,clip,std::move(f));}
std::wstring settingActionLabel(const std::string& key){if(key=="content_mode")return hardcore()?L"하드코어 ON":L"하드코어 OFF";if(key=="server")return customServerEdition?L"서버 주소 변경":L"서버 변경…";if(key=="reset")return L"초기화…";return L"실행";}
float settingCardHeight(const SettingDef& d){return d.key=="_about"?150.f:66.f;}
void drawAboutCard(Rect cr){renderer.box(cr,0x202023,0x2d2d32,9);if(!studioBrand)studioBrand=renderer.resource(204);renderer.bitmap(studioBrand.Get(),{cr.x+22,cr.y+20,48,48});renderer.text(L"White Studio",{cr.x+86,cr.y+16,cr.w-110,28},0xf2f2f4,20,true,false);renderer.text(L"SNUWCANVAS "+wide(CLIENT_DISPLAY_VERSION),{cr.x+86,cr.y+46,cr.w-110,22},0x9a9aa6,12,false,false);renderer.fill({cr.x+22,cr.y+82,cr.w-44,1},0x2d2d32);renderer.text(L"Copyright © White Studio. All rights reserved.",{cr.x+22,cr.y+90,cr.w-44,22},0xd8d8de,12,true,false);renderer.text(L"이 소프트웨어의 모든 권리는 White Studio에 있으며, 사전 서면 허가 없이 복제·배포·수정·역설계할 수 없습니다.",{cr.x+22,cr.y+112,cr.w-44,30},0x8b8b96,11,false,false);}
void drawSettingCard(const SettingDef& d,Rect cr,Rect clip){
 if(cr.y+cr.h<clip.y||cr.y>clip.y+clip.h)return;if(d.key=="_about"){drawAboutCard(cr);return;}
 const auto& cats=settingCategories();bool hov=sHover(cr,clip);float ht=animate(sKey(d.key,1),hov?1.f:0.f,100.f);
 renderer.box(cr,mixColor(0x202023,0x26262a,ht),mixColor(0x2d2d32,0x3c3c44,ht),9);
 bool mod=settingModified(d);float mt=animate(sKey(d.key,2),mod?1.f:0.f,120.f);if(mt>.01f)renderer.box({cr.x,cr.y+12,3,cr.h-24},mixColor(0x202023,0xb5c2ff,mt),0,1.5f);
 icon({cr.x+18,cr.y+22,22,22},cats[d.category].icon,mixColor(0x6f7aa8,0xb5c2ff,ht));
 float rr=cr.x+cr.w-16;bool canReset=settingResettable(d);float ctrlRight=rr-(canReset?34:0),ctrlLeft=ctrlRight;
 if(canReset&&mod){Rect rb{rr-26,cr.y+20,26,26};float t=animate(sKey(d.key,3),sHover(rb,clip)?1.f:0.f,90.f);renderer.box(rb,mixColor(0x2a2a2f,0x3c3c44,t),0,7);icon({rb.x+5,rb.y+5,16,16},48,mixColor(0x9aa4d8,0xe9ecff,t));sHit(rb,clip,[this,k=d.key]{applySetting(k,settingDefault(k));});}
 std::wstring desc=d.desc,value;bool keyInfo=d.key.rfind("_k_",0)==0;
 switch(d.type){
  case SettingType::Toggle:{bool on=flag(store->settings,d.key);Rect tr{ctrlRight-46,cr.y+21,46,24};float a=animate(sKey(d.key,4),on?1.f:0.f,85.f);renderer.box(tr,mixColor(0x3a3a41,0xb5c2ff,a),0,12);renderer.box({tr.x+3+(tr.w-24)*a,tr.y+3,18,18},mixColor(0xcfcfd6,0x15151a,a*.15f),0,9);sHit({tr.x-6,cr.y+8,tr.w+12,cr.h-16},clip,[this,k=d.key,on]{applySetting(k,!on);});ctrlLeft=tr.x;break;}
  case SettingType::Choice:{float chip=96,x0=ctrlRight-float(d.options.size())*(chip+4)+4;ctrlLeft=x0;auto cur=str(store->settings,d.key);for(size_t i=0;i<d.options.size();i++){Rect c{x0+float(i)*(chip+4),cr.y+19,chip,28};bool sel=cur==d.options[i].first;float a=animate(sKey(d.key,10+int(i)),sel?1.f:0.f,90.f);bool over=sHover(c,clip);renderer.box(c,mixColor(mixColor(0x2a2a2f,0x34343a,over?1.f:0.f),0xb5c2ff,a),mixColor(0x3b3b40,0xb5c2ff,a),7);renderer.text(d.options[i].second,c,mixColor(0xcfcfd6,0x111111,a),12,false,true);sHit(c,clip,[this,k=d.key,v=d.options[i].first]{applySetting(k,v);});}break;}
  case SettingType::Number:case SettingType::Text:{Rect vb{ctrlRight-220,cr.y+17,220,32};ctrlLeft=vb.x;bool editing=settingsEdit&&settingsEditKey==d.key;bool over=sHover(vb,clip);float a=animate(sKey(d.key,5),(over||editing)?1.f:0.f,90.f);renderer.box(vb,0x141416,editing?0xb5c2ff:mixColor(0x3b3b40,0x6a6a78,a),7);auto text=wide(settingText(store->settings.value(d.key,json())));if(!editing)renderer.text(text.empty()?L"(비어 있음)":sFit(text,12,vb.w-22),{vb.x+11,vb.y,vb.w-22,vb.h},text.empty()?0x66666d:0xe5e5e7,12,false,false);sHit(vb,clip,[this,def=&d,vb]{beginSettingsEdit(*def,vb);});break;}
  case SettingType::Action:{Rect ab{rr-168,cr.y+17,168,32};ctrlLeft=ab.x;sButton(ab,clip,d.key,settingActionLabel(d.key),d.key=="reset"?0xff8f8f:0xe5e5e7,[this,k=d.key]{settingsAction(k);});break;}
  case SettingType::Info:{value=keyInfo?d.desc:d.key=="api"?wide(str(store->settings,"api")):d.key=="_version"?wide(CLIENT_DISPLAY_VERSION):wide(str(store->settings,d.key));if(keyInfo)desc.clear();float w=std::min(sTextW(value,12),std::max(120.f,cr.w*.5f));value=sFit(value,12,w);Rect vr{ctrlRight-w,cr.y,w,cr.h};renderer.text(value,vr,0xb4b4bd,12,false,false);ctrlLeft=vr.x;break;}}
 float textW=std::max(80.f,ctrlLeft-(cr.x+54)-18);
 renderer.text(sFit(d.label,14,textW),{cr.x+54,cr.y+8,textW,26},0xe9e9ec,14,false,false);
 if(!desc.empty())renderer.text(sFit(desc,11,textW),{cr.x+54,cr.y+34,textW,22},0x8b8b96,11,false,false);
 if(d.restart)renderer.text(L"재시작 필요",{cr.x+cr.w-140,cr.y+2,130,14},0xe0b45c,10,false,false);}
void paintSettings(){
 if(!settingsOpen)return;
 settingsHits.clear();hideForSettings();const auto& cats=settingCategories();const auto& reg=settingRegistry();
 float open=animate(animKey({0,0,3,3},31),1.f,70.f);
 if(std::fabs(settingsScrollTarget-settingsScroll)>.4f){if(!motionEnabled())settingsScroll=settingsScrollTarget;else{settingsScroll+=(settingsScrollTarget-settingsScroll)*(1.f-std::exp(-animDt/70.f));animPending=true;}}else settingsScroll=settingsScrollTarget;
 renderer.fill({0,0,width,height},0x000000,.62f*open);
 const float W=std::max(560.f,std::min(1060.f,width-64)),H=std::max(380.f,std::min(720.f,height-64));
 Rect win{(width-W)/2,(height-H)/2+(1.f-open)*18.f,W,H};settingsWin=win;Rect all{-100000,-100000,200000,200000};
 renderer.box({win.x-1,win.y-1,win.w+2,win.h+2},0x111113,0,13);renderer.box(win,0x1b1b1d,0x3a3a40,12);
 // 상단 바
 icon({win.x+22,win.y+18,22,22},40,0xb5c2ff);renderer.text(L"설정",{win.x+54,win.y+10,70,38},0xf2f2f4,19,true,false);
 int modifiedCount=0;for(auto& d:reg)if(settingModified(d))modifiedCount++;
 if(modifiedCount>0)renderer.text(L"수정됨 "+std::to_wstring(modifiedCount)+L"개",{win.x+128,win.y+10,120,38},0x8b8b96,12,false,false);
 Rect cb{win.x+win.w-46,win.y+13,32,32};{float t=animate(animKey(cb,41),sHover(cb,all)?1.f:0.f,90.f);renderer.box(cb,mixColor(0x1b1b1d,0x34343a,t),0,8);icon({cb.x+8,cb.y+8,16,16},50,mixColor(0x9a9aa6,0xffffff,t));sHit(cb,all,[this]{closeSettings();});}
 Rect sb{cb.x-12-300,win.y+13,300,32};renderer.box(sb,0x141416,GetFocus()==settingsSearch?0xb5c2ff:0x3b3b40,8);icon({sb.x+10,sb.y+8,16,16},47,0x8b8b96);
 if(settingsSearch)place(settingsSearch,{sb.x+34,sb.y+6,sb.w-64,20});
 if(!settingsQuery.empty()){Rect xb{sb.x+sb.w-28,sb.y+6,20,20};icon({xb.x+2,xb.y+2,16,16},50,0x9a9aa6);sHit(xb,all,[this]{if(settingsSearch)SetWindowTextW(settingsSearch,L"");});}
 renderer.fill({win.x,win.y+58,win.w,1},0x2a2a2e);
 // 사이드바
 const float SW=212;renderer.fill({win.x+SW,win.y+59,1,H-59},0x2a2a2e);
 for(size_t i=0;i<cats.size();i++){Rect row{win.x+10,win.y+70+float(i)*42,SW-20,38};bool sel=settingsQuery.empty()&&int(i)==settingsCat;bool over=sHover(row,all);int hits=0;if(!settingsQuery.empty())for(auto& d:reg)if(d.category==int(i)&&settingMatches(d,settingsQuery))hits++;
  float sa=animate(animKey(row,42),sel?1.f:0.f,100.f),ha=animate(animKey(row,43),over?1.f:0.f,90.f);bool dim=!settingsQuery.empty()&&hits==0;
  renderer.box(row,mixColor(mixColor(0x1b1b1d,0x242428,ha),0x2b2b33,sa),0,8);if(sa>.01f)renderer.box({row.x,row.y+19-9*sa,3,18*sa},0xb5c2ff,0,1.5f);
  icon({row.x+14,row.y+10,18,18},cats[i].icon,dim?0x55555d:mixColor(0x8b8fb0,0xb5c2ff,std::max(sa,ha*.6f)));renderer.text(cats[i].name,{row.x+44,row.y,row.w-80,row.h},dim?0x66666d:mixColor(0xcfcfd6,0xffffff,sa),13,sel,false);
  if(!settingsQuery.empty()&&hits>0)renderer.text(std::to_wstring(hits),{row.x+row.w-34,row.y,26,row.h},0x9aa4d8,12,true,true);
  sHit(row,all,[this,i]{if(settingsSearch)SetWindowTextW(settingsSearch,L"");settingsCat=int(i);settingsScroll=settingsScrollTarget=0;});}
 {Rect lr{win.x+10,win.y+H-50,SW-20,36};float t=animate(animKey(lr,44),sHover(lr,all)?1.f:0.f,90.f);renderer.box(lr,mixColor(0x1b1b1d,0x242428,t),mixColor(0x2d2d32,0x3c3c44,t),8);renderer.text(L"이전 설정 화면",lr,mixColor(0x8b8b96,0xd0d0d8,t),12,false,true);sHit(lr,all,[this]{closeSettings();setPage(10);});}
 // 본문
 Rect vp{win.x+SW+1,win.y+59,win.w-SW-1,H-59};settingsViewH=vp.h;float cx=vp.x+28,cw=std::min(780.f,vp.w-64);
 float y=vp.y+16-settingsScroll,startY=y;bool any=false;
 renderer.target->PushAxisAlignedClip(vp.d2d(),D2D1_ANTIALIAS_MODE_ALIASED);
 for(size_t ci=0;ci<cats.size();ci++){
  if(settingsQuery.empty()&&int(ci)!=settingsCat)continue;
  std::vector<const SettingDef*> items;for(auto& d:reg)if(d.category==int(ci)&&settingMatches(d,settingsQuery))items.push_back(&d);
  if(items.empty())continue;any=true;
  renderer.text(cats[ci].name,{cx,y,300,32},0xf2f2f4,20,true,false);renderer.text(cats[ci].hint,{cx+2,y+30,360,20},0x8b8b96,12,false,false);
  if(cats[ci].savesDefaults&&settingsQuery.empty()){Rect b2{cx+cw-120,y+6,120,30},b1{b2.x-8-200,y+6,200,30};sButton(b1,vp,"_save"+std::to_string(ci),L"현재 값을 기본값으로 저장",0xe5e5e7,[this,c=int(ci)]{settingsSaveDefaults(c);});sButton(b2,vp,"_restore"+std::to_string(ci),L"기본값 복원",0xe5e5e7,[this,c=int(ci)]{settingsRestoreDefaults(c);});}
  y+=64;std::wstring section;
  for(auto* d:items){if(d->section!=section){section=d->section;renderer.text(section,{cx+2,y,400,26},0x9aa4d8,12,true,false);y+=30;}float ch=settingCardHeight(*d);drawSettingCard(*d,{cx,y,cw,ch},vp);y+=ch+8;}
  y+=16;}
 if(!any){icon({vp.x+vp.w/2-14,vp.y+vp.h/2-40,28,28},47,0x66666d);renderer.text(L"'"+settingsQuery+L"'와 일치하는 설정이 없습니다.",{vp.x,vp.y+vp.h/2-4,vp.w,28},0x8b8b96,13,false,true);}
 renderer.target->PopAxisAlignedClip();
 settingsContent=(y-startY)+24;float limit=std::max(0.f,settingsContent-vp.h);settingsScrollTarget=std::clamp(settingsScrollTarget,0.f,limit);settingsScroll=std::clamp(settingsScroll,0.f,limit);
 if(limit>0){float th=std::max(36.f,vp.h*vp.h/settingsContent),ty=vp.y+(vp.h-th)*(settingsScroll/limit);renderer.box({vp.x+vp.w-9,ty,4,th},0x56565e,0,2);}
 if(GetTickCount64()<settingsNoticeUntil){float tw=std::max(200.f,sTextW(settingsNotice,13)+40);Rect pill{win.x+(win.w-tw)/2,win.y+H-58,tw,36};renderer.box({pill.x,pill.y+2,pill.w,pill.h},0x0e0e10,0,18);renderer.box(pill,0x2a2a31,0x4a4a55,18);renderer.text(settingsNotice,pill,0xf2f2f4,13,false,true);animPending=true;}
}
