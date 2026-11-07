// Members of App. Names only: typed prompt text is never sent for autocomplete.
HWND entityList=nullptr;
std::thread entityThread;std::atomic_bool entityCancel{false};bool entityRunning=false,entityLoaded=false;
ULONGLONG entityRefreshAt=0;std::string entityCacheScope,entityDismissed;
json entityNames=json::array();std::vector<std::wstring> entityMatches;EntityPrefix entityRange;LONG entityCaret=0;
std::string entityScope()const{return str(store->settings,"api")+"|"+store->workspaceScope+"|"+sha256(str(store->settings,"api_key"));}
void hideEntitySuggestions(bool dismissed=false){if(dismissed)entityDismissed=promptDisplayed;if(entityList){auto window=entityList;entityList=nullptr;DestroyWindow(window);}entityMatches.clear();}
void loadEntityNames(){
 if(entityRunning||!developer()||str(store->settings,"api_key").empty())return;
 if(entityThread.joinable())entityThread.join();auto cfg=store->settings;auto scope=entityScope();auto* target=engine;entityRunning=true;entityCancel=false;
 entityThread=std::thread([this,cfg,scope,target]{json payload={{"scope",scope},{"ok",false}};
  try{NetworkScope mode(&entityCancel,std::chrono::steady_clock::now()+std::chrono::seconds(5),str(cfg,"content_mode","standard"));auto response=request("GET",joinUrl(str(cfg,"api"),"/learning/profiles/suggestions?limit=100"),str(cfg,"api_key"),"","application/json",&entityCancel,65536,5000);if(response.code<300){payload["names"]=response.object().value("suggestions",json::array());payload["ok"]=true;}}
  catch(...){}target->event({"entity_names","","",payload});
 });
}
void entityNamesReply(const Event& event){entityRunning=false;if(str(event.data,"scope")!=entityScope()){if(entityCacheScope!=entityScope()){entityNames=json::array();entityLoaded=false;entityCacheScope=entityScope();entityRefreshAt=0;}if(hwnd)SetTimer(hwnd,3,160,nullptr);return;}entityCacheScope=str(event.data,"scope");entityNames=event.data.value("names",json::array());if(!entityNames.is_array()||entityNames.size()>100)entityNames=json::array();entityLoaded=true;entityRefreshAt=GetTickCount64()+60000;if(hwnd)SetTimer(hwnd,3,160,nullptr);}
void acceptEntity(size_t index){
 if(index>=entityMatches.size()||!positive||promptComposing||!developer())return;
 CHARRANGE selection{};SendMessageW(positive,EM_EXGETSEL,0,(LPARAM)&selection);if(selection.cpMin!=entityCaret||selection.cpMax!=entityCaret){hideEntitySuggestions();return;}
 std::wstring before(size_t(entityCaret)+1,L'\0');TEXTRANGEW beforeRange{{0,entityCaret},before.data()};auto length=SendMessageW(positive,EM_GETTEXTRANGE,0,(LPARAM)&beforeRange);before.resize(size_t(std::clamp(length,LRESULT(0),LRESULT(entityCaret))));auto current=entityPrefix(before);if(!current.valid||current.begin!=entityRange.begin||current.query!=entityRange.query){hideEntitySuggestions();return;}
 auto inserted=L":"+entityMatches[index]+L":";CHARRANGE replace{LONG(entityRange.begin),entityCaret};hideEntitySuggestions();
 wchar_t tail[2]{};TEXTRANGEW tailRange{{entityCaret,entityCaret+1},tail};SendMessageW(positive,EM_GETTEXTRANGE,0,(LPARAM)&tailRange);if(tail[0]==L':')replace.cpMax++;
 SendMessageW(positive,EM_EXSETSEL,0,(LPARAM)&replace);SendMessageW(positive,EM_REPLACESEL,TRUE,(LPARAM)inserted.c_str());SetFocus(positive);syncPromptEditor();entityDismissed=promptDisplayed;
}
static LRESULT CALLBACK entityListProc(HWND window,UINT message,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR data){
 auto* app=reinterpret_cast<App*>(data);
 if(message==WM_LBUTTONUP){auto item=SendMessageW(window,LB_ITEMFROMPOINT,0,b);if(!HIWORD(item))app->acceptEntity(LOWORD(item));return 0;}
 if(message==WM_KEYDOWN&&a==VK_RETURN){auto selected=SendMessageW(window,LB_GETCURSEL,0,0);app->acceptEntity(selected==LB_ERR?0:size_t(selected));return 0;}
 if(message==WM_KEYDOWN&&a==VK_ESCAPE){app->hideEntitySuggestions(true);SetFocus(app->positive);return 0;}
 if(message==WM_NCDESTROY){if(app->entityList==window)app->entityList=nullptr;RemoveWindowSubclass(window,entityListProc,id);}
 return DefSubclassProc(window,message,a,b);
}
void showEntitySuggestions(const EntityPrefix& prefix,LONG caret,const std::vector<std::wstring>& matches){
 hideEntitySuggestions();if(matches.empty())return;entityMatches=matches;entityRange=prefix;entityCaret=caret;
 POINT point{};GetCaretPos(&point);MapWindowPoints(positive,hwnd,&point,1);RECT area{};GetClientRect(hwnd,&area);int w=std::min(int(310*dpi),int(std::max(160L,area.right-16)));int h=int((std::min(size_t(6),matches.size())*22+8)*dpi);point.x=std::clamp(point.x,8L,std::max(8L,area.right-w-8));point.y+=int(22*dpi);if(point.y+h>area.bottom)point.y=std::max(8L,point.y-h-int(24*dpi));
 entityList=CreateWindowExW(0,L"LISTBOX",L"지칭어 자동완성",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|LBS_HASSTRINGS,point.x,point.y,w,h,hwnd,(HMENU)897,GetModuleHandle(nullptr),nullptr);if(!entityList){entityMatches.clear();return;}
 SetWindowTheme(entityList,L"DarkMode_Explorer",nullptr);SendMessageW(entityList,WM_SETFONT,(WPARAM)uiFont,FALSE);SetWindowSubclass(entityList,entityListProc,20,(DWORD_PTR)this);for(auto& name:matches)SendMessageW(entityList,LB_ADDSTRING,0,(LPARAM)name.c_str());SetWindowPos(entityList,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}
void updateEntitySuggestions(){
 if(entityList&&GetFocus()==entityList)return;
 if(page!=0||!positive||GetFocus()!=positive||promptComposing||!developer()||interactiveResize||modelId(store->settings)=="pixel_art"){hideEntitySuggestions();return;}
 CHARRANGE selection{};SendMessageW(positive,EM_EXGETSEL,0,(LPARAM)&selection);if(selection.cpMin<0||selection.cpMin!=selection.cpMax||selection.cpMin>65536){hideEntitySuggestions();return;}
 std::wstring prefix(size_t(selection.cpMin)+1,L'\0');TEXTRANGEW range{{0,selection.cpMin},prefix.data()};auto length=SendMessageW(positive,EM_GETTEXTRANGE,0,(LPARAM)&range);prefix.resize(size_t(std::clamp(length,LRESULT(0),LRESULT(selection.cpMin))));auto completion=entityPrefix(prefix);
 if(!completion.valid||entityDismissed==promptDisplayed){hideEntitySuggestions();return;}
 auto scope=entityScope();if(entityCacheScope!=scope){entityCancel=true;entityNames=json::array();entityLoaded=false;entityCacheScope=scope;entityRefreshAt=0;}
 if(!entityLoaded||GetTickCount64()>=entityRefreshAt){loadEntityNames();if(!entityLoaded){hideEntitySuggestions();return;}}
 auto matches=matchingEntities(entityNames,completion.query);if(entityList&&entityCaret==selection.cpMin&&entityRange.begin==completion.begin&&matches==entityMatches)return;showEntitySuggestions(completion,selection.cpMin,matches);
}
bool entityKey(WPARAM key){
 if(!entityList||promptComposing||(GetKeyState(VK_CONTROL)&0x8000)||(GetKeyState(VK_SHIFT)&0x8000))return false;
 if(key==VK_ESCAPE){hideEntitySuggestions(true);return true;}
 auto selected=SendMessageW(entityList,LB_GETCURSEL,0,0);
 if(key==VK_RETURN){acceptEntity(selected==LB_ERR?0:size_t(selected));return true;}
 if(key==VK_UP||key==VK_DOWN){int next=selected==LB_ERR?0:std::clamp(int(selected)+(key==VK_UP?-1:1),0,int(entityMatches.size())-1);SendMessageW(entityList,LB_SETCURSEL,next,0);return true;}
 return false;
}
