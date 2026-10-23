HWND profileNickname=nullptr,profileBio=nullptr;
void positionAccountControls(Rect c){
 float line=22;if(hwnd){auto dc=GetDC(hwnd);auto old=SelectObject(dc,uiFont);TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);SelectObject(dc,old);ReleaseDC(hwnd,dc);line=metrics.tmHeight/dpi;}
 place(profileNickname,{c.x+18,206+(40-line)/2,c.w-36,line});place(profileBio,{c.x+18,282,c.w-36,110});
}
void accountControls(Rect c){
 profileNickname=control(L"EDIT",wide(str(accountProfile,"nickname")),ES_AUTOHSCROLL,1201);SendMessageW(profileNickname,EM_SETLIMITTEXT,32,0);
 profileBio=control(L"EDIT",wide(str(accountProfile,"bio")),ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,1202);SendMessageW(profileBio,EM_SETLIMITTEXT,500,0);
 positionAccountControls(c);
}
void accountPage(Rect c){
 renderer.box({c.x+16,90,54,54},0x34343b,0,14);icon({c.x+30,102,26,26},5,0xb5c2ff);
 label(wide(str(accountProfile,"nickname","내 프로필")),{c.x+86,90,c.w-106,28},true,0xe5e5e7,18);
 label(wide(str(accountProfile,"email")),{c.x+86,123,c.w-106,23},false,0x99999f,11);
 label(L"닉네임",{c.x+18,180,c.w-36,26},true);renderer.box({c.x+14,206,c.w-28,40},0x161617,0x333338,6);
 label(L"자기소개",{c.x+18,252,c.w-36,26},true);renderer.box({c.x+14,278,c.w-28,118},0x161617,0x333338,6);
 button({c.x+18,410,c.w-36,36},L"프로필 저장",[this]{featureRequest("account_profile","PATCH","/api/auth/me",json{{"nickname",utf8(textOf(profileNickname))},{"bio",normalizedText(profileBio)}});},true,!learningBusy);
 label(L"권한  ·  "+wide(str(accountProfile,"role")),{c.x+18,470,c.w-36,26});
 auto total=quotaUsage.value("total",json());label(total.is_number()?L"남은 생성  ·  "+std::to_wstring(quotaUsage.value("remaining",int64_t(0)))+L" / "+std::to_wstring(total.get<int64_t>())+L"장":L"사용 한도  ·  무제한",{c.x+18,506,c.w-36,26},false,0x99999f,12);
 bool ready=!accountWorkspaceBusy();
 button({c.x+18,560,c.w-36,34},L"로그아웃",[this]{featureRequest("account_logout","POST",accountLogoutPath(false));},false,ready);
 button({c.x+18,606,c.w-36,34},L"모든 기기에서 로그아웃",[this]{if(MessageBoxW(hwnd,L"모든 기기의 로그인 세션을 종료할까요?",L"로그아웃",MB_YESNO|MB_ICONQUESTION)==IDYES)featureRequest("account_logout","POST",accountLogoutPath(true));},false,ready);
}
