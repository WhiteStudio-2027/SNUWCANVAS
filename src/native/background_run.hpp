// 창을 닫아도 예약된 이미지 생성을 이어서 진행하는 백그라운드 운영. App 클래스 본문 안에서 include 된다.
bool backgroundMode=false,forceQuit=false;ULONGLONG backgroundDoneAt=0;HANDLE backgroundMutex=nullptr;int backgroundTipPercent=-1;
static std::wstring backgroundMutexName(){return std::wstring(L"Local\\SNUWCANVAS.Background")+(customServerEdition?L".Custom":L"");}
bool generationActive(){std::lock_guard lock(engine->mutex);return engine->busy||!engine->tasks.empty();}
bool shouldRunInBackground(){return !forceQuit&&!closing&&hwnd&&flag(store->settings,"background_run")&&generationActive();}
void trayTip(const std::wstring& tip){NOTIFYICONDATAW n{sizeof n};n.hWnd=hwnd;n.uID=1;n.uFlags=NIF_TIP;wcsncpy_s(n.szTip,tip.c_str(),_TRUNCATE);Shell_NotifyIconW(NIM_MODIFY,&n);}
void trayBalloon(const std::wstring& title,const std::wstring& text){NOTIFYICONDATAW n{sizeof n};n.hWnd=hwnd;n.uID=1;n.uFlags=NIF_INFO;n.dwInfoFlags=NIIF_INFO;wcsncpy_s(n.szInfoTitle,title.c_str(),_TRUNCATE);wcsncpy_s(n.szInfo,text.c_str(),_TRUNCATE);Shell_NotifyIconW(NIM_MODIFY,&n);}
void removeTray(){if(!backgroundMode)return;NOTIFYICONDATAW n{sizeof n};n.hWnd=hwnd;n.uID=1;Shell_NotifyIconW(NIM_DELETE,&n);if(backgroundMutex){CloseHandle(backgroundMutex);backgroundMutex=nullptr;}}
void enterBackground(){try{save();}catch(...){}closeSettings();backgroundMode=true;backgroundDoneAt=0;backgroundTipPercent=-1;
 NOTIFYICONDATAW n{sizeof n};n.hWnd=hwnd;n.uID=1;n.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;n.uCallbackMessage=WM_APP+7;n.hIcon=LoadIconW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(1));wcsncpy_s(n.szTip,L"SNUWCANVAS · 백그라운드에서 이미지 생성 중",_TRUNCATE);Shell_NotifyIconW(NIM_ADD,&n);
 backgroundMutex=CreateMutexW(nullptr,FALSE,backgroundMutexName().c_str());ShowWindow(hwnd,SW_HIDE);
 trayBalloon(L"백그라운드에서 생성 중",L"창을 닫아도 예약한 이미지 생성은 계속됩니다. 아이콘을 누르면 다시 열 수 있어요.");}
void restoreFromBackground(){if(!backgroundMode)return;removeTray();backgroundMode=false;backgroundDoneAt=0;ShowWindow(hwnd,IsIconic(hwnd)?SW_RESTORE:SW_SHOW);SetForegroundWindow(hwnd);childrenStale=true;InvalidateRect(hwnd,nullptr,FALSE);}
void backgroundTick(){if(!backgroundMode)return;if(generationActive()){backgroundDoneAt=0;int pct=int(std::clamp(progress,0.f,100.f));if(pct!=backgroundTipPercent){backgroundTipPercent=pct;trayTip(L"SNUWCANVAS · 생성 중 "+std::to_wstring(pct)+L"%");}return;}
 if(!backgroundDoneAt){backgroundDoneAt=GetTickCount64();trayTip(L"SNUWCANVAS · 생성 완료");trayBalloon(L"이미지 생성이 끝났습니다",L"갤러리에서 결과를 확인하세요. 잠시 후 자동으로 종료됩니다.");}
 else if(GetTickCount64()-backgroundDoneAt>9000){forceQuit=true;PostMessageW(hwnd,WM_CLOSE,0,0);}}
void trayMenu(){HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,L"창 열기");AppendMenuW(menu,MF_STRING,2,generationActive()?L"생성 취소하고 종료":L"종료");POINT cursor;GetCursorPos(&cursor);SetForegroundWindow(hwnd);int choice=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,cursor.x,cursor.y,0,hwnd,nullptr);DestroyMenu(menu);if(choice==1)restoreFromBackground();else if(choice==2){forceQuit=true;PostMessageW(hwnd,WM_CLOSE,0,0);}}
