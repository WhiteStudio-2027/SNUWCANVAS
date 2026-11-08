HWND profileNickname=nullptr,profileBio=nullptr;
#include "account_autosave.hpp"
#include "account_media.hpp"
float accountMediaOffset(){return accountMediaSupported()?180.f:0.f;}
Rect accountViewport(Rect c){return {c.x+8,80,c.w-16,std::max(40.f,height-96)};}
float accountScrollMax(){return std::max(0.f,656+accountMediaOffset()-height);}
void positionAccountControls(Rect c){
 accountScroll=std::clamp(accountScroll,0.f,accountScrollMax());float line=22;if(hwnd){auto dc=GetDC(hwnd);auto old=SelectObject(dc,uiFont);TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);SelectObject(dc,old);ReleaseDC(hwnd,dc);line=metrics.tmHeight/dpi;}
 Rect nickname{c.x+18,206+accountMediaOffset()+(40-line)/2-accountScroll,c.w-36,line},bio{c.x+18,282+accountMediaOffset()-accountScroll,c.w-36,110};place(profileNickname,nickname);place(profileBio,bio);auto area=accountViewport(c);for(auto [window,rect]:std::vector<std::pair<HWND,Rect>>{{profileNickname,nickname},{profileBio,bio}})if(window)ShowWindow(window,rect.y>=area.y&&rect.y+rect.h<=area.y+area.h?SW_SHOW:SW_HIDE);
}
void accountControls(Rect c){
 bindAccountDraft();profileNickname=control(L"EDIT",wide(str(accountDraft,"nickname")),ES_AUTOHSCROLL,1201);SendMessageW(profileNickname,EM_SETLIMITTEXT,32,0);SetWindowSubclass(profileNickname,accountProfileEditProc,51,(DWORD_PTR)this);
 profileBio=control(L"EDIT",wide(str(accountDraft,"bio")),ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,1202);SendMessageW(profileBio,EM_SETLIMITTEXT,500,0);SetWindowSubclass(profileBio,accountProfileEditProc,51,(DWORD_PTR)this);
 positionAccountControls(c);
}
void accountPage(Rect c){
 auto area=accountViewport(c);renderer.target->PushAxisAlignedClip(area.d2d(),D2D1_ANTIALIAS_MODE_ALIASED);auto y=[&](float value){return value+accountMediaOffset()-accountScroll;};auto action=[&](float top,float x,float width,const std::wstring& title,std::function<void()> command,bool enabled=true){Rect rect{x,y(top),width,32};if(rect.y>=area.y&&rect.y+rect.h<=area.y+area.h)button(rect,title,command,false,enabled);};
 if(accountMediaSupported()){float top=90-accountScroll;Rect banner{c.x+18,top,c.w-36,100};renderer.box(banner,0x242428,0x3a3a40,6);renderer.bitmap(accountMediaBitmaps[1].Get(),banner);Rect avatar{c.x+18,top+105,48,48};renderer.box(avatar,0x34343b,0,8);renderer.bitmap(accountMediaBitmaps[0].Get(),avatar);bool ready=!accountMediaBusy&&!accountMediaActive&&!accountSaving&&!accountSaveWorkerActive;float mediaWidth=std::max(40.f,(c.w-104)/3);action(25,c.x+76,mediaWidth,L"프로필",[this]{accountMediaMenu("avatar");},ready);action(25,c.x+82+mediaWidth,mediaWidth,L"배너",[this]{accountMediaMenu("banner");},ready);action(25,c.x+88+2*mediaWidth,mediaWidth,accountMediaRetry.empty()?L"새로고침":L"재시도",[this]{startAccountMedia(accountMediaRetry);},ready);label(accountMediaMessage,{c.x+18,top+154,c.w-36,22},false,0xccccd0,11);}
 renderer.box({c.x+16,y(90),54,54},0x34343b,0,14);icon({c.x+30,y(102),26,26},5,0xb5c2ff);
 label(wide(str(accountProfile,"nickname","내 프로필")),{c.x+86,y(90),c.w-106,28},true,0xe5e5e7,18);
 label(wide(str(accountProfile,"email")),{c.x+86,y(123),c.w-106,23},false,0xccccd0,11);
 label(L"닉네임",{c.x+18,y(180),c.w-36,26},true);renderer.box({c.x+14,y(206),c.w-28,40},0x161617,0x333338,6);
 label(L"자기소개",{c.x+18,y(252),c.w-36,26},true);renderer.box({c.x+14,y(278),c.w-28,118},0x161617,0x333338,6);
 label(accountSaveMessage,{c.x+18,y(400),c.w-36,25},false,accountSaveFailed?0xf0b3b3:0xccccd0,11);float half=(c.w-44)/2;
 action(430,c.x+18,half,accountSaveFailed?L"다시 시도":L"저장",[this]{accountProfileEdited();sendAccountProfile(false,true);},accountDraftDirty&&!accountSaving&&!accountSaveWorkerActive);
 action(430,c.x+26+half,half,L"다시 불러오기",[this]{if(accountDraftDirty&&MessageBoxW(hwnd,L"현재 입력은 초안으로 보관하고 서버 프로필을 다시 불러올까요?",L"프로필",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;sendAccountProfile(true,true);},!accountSaving&&!accountSaveWorkerActive);
 label(L"권한  ·  "+wide(str(accountProfile,"role")),{c.x+18,y(470),c.w-36,26});
 auto total=quotaUsage.value("total",json());label(total.is_number()?L"남은 생성  ·  "+std::to_wstring(quotaUsage.value("remaining",int64_t(0)))+L" / "+std::to_wstring(total.get<int64_t>())+L"장":L"사용 한도  ·  무제한",{c.x+18,y(506),c.w-36,26},false,0xccccd0,12);
 bool ready=!accountWorkspaceBusy();action(560,c.x+18,c.w-36,L"로그아웃",[this]{featureRequest("account_logout","POST",accountLogoutPath(false));},ready);
 action(606,c.x+18,c.w-36,L"모든 기기에서 로그아웃",[this]{if(MessageBoxW(hwnd,L"모든 기기의 로그인 세션을 종료할까요?",L"로그아웃",MB_YESNO|MB_ICONQUESTION)==IDYES)featureRequest("account_logout","POST",accountLogoutPath(true));},ready);
 renderer.target->PopAxisAlignedClip();if(accountScrollMax()>0){float thumb=area.h*area.h/(area.h+accountScrollMax());renderer.box({area.x+area.w-3,area.y+(area.h-thumb)*accountScroll/accountScrollMax(),3,thumb},0x707078,0,1);}
}
