HWND profileNickname=nullptr,profileBio=nullptr;
#include "account_autosave.hpp"
#include "account_media.hpp"
// 프로필 화면: 배너 위에 프로필 사진이 겹치는 소셜 프로필 형태. 배너와 사진은 눌러서 바꾼다.
struct AccountLayout{float top,bannerH,avatar,avatarY,chipsY,mediaY,cardY,nickBoxY,bioBoxY,statusY,buttonsY,logoutY,bottom;};
Rect accountViewport(Rect c){return {c.x+8,80,c.w-16,std::max(40.f,height-96)};}
AccountLayout accountLayout(Rect c){AccountLayout l{};l.top=90;l.bannerH=std::clamp((c.w-36)/4.f,64.f,150.f);l.avatar=std::clamp(c.w*.16f,52.f,88.f);l.avatarY=l.top+l.bannerH-l.avatar/2;
 l.chipsY=l.top+l.bannerH+std::max(l.avatar/2+20,68.f);l.mediaY=l.chipsY+34;l.cardY=l.mediaY+28;l.nickBoxY=l.cardY+70;l.bioBoxY=l.cardY+144;l.statusY=l.cardY+262;l.buttonsY=l.cardY+288;l.logoutY=l.cardY+354;l.bottom=l.logoutY+132+24;return l;}
float accountScrollMax(){return std::max(0.f,accountLayout(center()).bottom-(height-16));}
void positionAccountControls(Rect c){
 accountScroll=std::clamp(accountScroll,0.f,accountScrollMax());float line=22;if(hwnd){auto dc=GetDC(hwnd);auto old=SelectObject(dc,uiFont);TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);SelectObject(dc,old);ReleaseDC(hwnd,dc);line=metrics.tmHeight/dpi;}
 auto l=accountLayout(c);Rect nickname{c.x+40,l.nickBoxY+(40-line)/2-accountScroll,c.w-80,line},bio{c.x+40,l.bioBoxY+8-accountScroll,c.w-80,96};place(profileNickname,nickname);place(profileBio,bio);auto area=accountViewport(c);for(auto [window,rect]:std::vector<std::pair<HWND,Rect>>{{profileNickname,nickname},{profileBio,bio}})if(window)ShowWindow(window,rect.y>=area.y&&rect.y+rect.h<=area.y+area.h?SW_SHOW:SW_HIDE);
}
void accountControls(Rect c){
 bindAccountDraft();profileNickname=control(L"EDIT",wide(str(accountDraft,"nickname")),ES_AUTOHSCROLL,1201);SendMessageW(profileNickname,EM_SETLIMITTEXT,32,0);SetWindowSubclass(profileNickname,accountProfileEditProc,51,(DWORD_PTR)this);
 profileBio=control(L"EDIT",wide(str(accountDraft,"bio")),ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL,1202);SendMessageW(profileBio,EM_SETLIMITTEXT,500,0);SetWindowSubclass(profileBio,accountProfileEditProc,51,(DWORD_PTR)this);
 positionAccountControls(c);
}
void accountPage(Rect c){
 auto area=accountViewport(c);auto l=accountLayout(c);renderer.target->PushAxisAlignedClip(area.d2d(),D2D1_ANTIALIAS_MODE_ALIASED);
 auto Y=[&](float v){return v-accountScroll;};bool media=accountMediaSupported();bool ready=!accountMediaBusy&&!accountMediaActive&&!accountSaving&&!accountSaveWorkerActive;
 auto hot=[&](Rect r,const std::wstring& name,std::function<void()> f,bool enabled){float x1=std::max(r.x,area.x),y1=std::max(r.y,area.y),x2=std::min(r.x+r.w,area.x+area.w),y2=std::min(r.y+r.h,area.y+area.h);if(x2>x1+1&&y2>y1+1)buttons.push_back({{x1,y1,x2-x1,y2-y1},name,std::move(f),false,enabled});};
 auto action=[&](float top,float x,float w,const std::wstring& title,std::function<void()> command,bool enabled=true){Rect rect{x,Y(top),w,34};if(rect.y>=area.y&&rect.y+rect.h<=area.y+area.h)button(rect,title,command,false,enabled);};
 auto card=[&](float top,float h){renderer.box({c.x+18,Y(top),c.w-36,h},0x1c1c1e,0x2d2d32,12);};
 // 배너
 Rect banner{c.x+18,Y(l.top),c.w-36,l.bannerH};bool bannerHot=media&&banner.hit(mouseX,mouseY)&&area.hit(mouseX,mouseY)&&!settingsOpen;float ba=animate(sKey("acct_banner",1),bannerHot?1.f:0.f,110.f);
 renderer.box(banner,0x202024,0x2d2d32,12);
 if(accountMediaBitmaps[1])renderer.bitmapCover(accountMediaBitmaps[1].Get(),banner,12);
 else{if(media){icon({banner.x+banner.w/2-9,banner.y+banner.h/2-22,18,18},51,0x7e88b8);renderer.text(L"배너 이미지 추가",{banner.x,banner.y+banner.h/2,banner.w,24},0x8b8b96,12,false,true);}}
 if(media&&ba>.02f){renderer.fill(banner,0x000000,.38f*ba);Rect pill{banner.x+banner.w-102,banner.y+banner.h-36,90,26};renderer.box(pill,0x26262b,0x4a4a55,13);icon({pill.x+10,pill.y+5,16,16},51,0xe5e5e7);renderer.text(L"배너 변경",{pill.x+30,pill.y,pill.w-34,pill.h},0xf2f2f4,11,false,false);}
 if(media)hot(banner,L"배너 사진 변경",[this]{accountMediaMenu("banner");},ready);
 // 프로필 사진 (배너 아래쪽에 겹침)
 float s=l.avatar;Rect avatar{c.x+30,Y(l.avatarY),s,s};bool avatarHot=media&&avatar.hit(mouseX,mouseY)&&area.hit(mouseX,mouseY)&&!settingsOpen;float aa=animate(sKey("acct_avatar",1),avatarHot?1.f:0.f,110.f);
 renderer.box({avatar.x-4,avatar.y-4,s+8,s+8},0x1b1b1c,0,s/2+4);
 if(accountMediaBitmaps[0])renderer.bitmapCover(accountMediaBitmaps[0].Get(),avatar,s/2);else{renderer.box(avatar,0x34343b,0,s/2);icon({avatar.x+s*.29f,avatar.y+s*.29f,s*.42f,s*.42f},5,0xb5c2ff);}
 if(media&&aa>.02f){renderer.brush->SetColor(color(0x000000,.5f*aa));renderer.target->FillRoundedRectangle(D2D1::RoundedRect(avatar.d2d(),s/2,s/2),renderer.brush.Get());icon({avatar.x+s*.3f,avatar.y+s*.3f,s*.4f,s*.4f},51,0xf2f2f4);}
 if(media)hot(avatar,L"프로필 사진 변경",[this]{accountMediaMenu("avatar");},ready);
 // 이름·이메일
 float nameX=avatar.x+s+16,nameW=std::max(40.f,c.x+c.w-18-nameX);
 label(wide(str(accountProfile,"nickname","내 프로필")),{nameX,Y(l.top+l.bannerH+8),nameW,30},true,0xf2f2f4,20);
 label(wide(str(accountProfile,"email")),{nameX,Y(l.top+l.bannerH+38),nameW,22},false,0x9a9aa6,11);
 // 권한·사용 한도
 {auto role=L"권한 · "+wide(str(accountProfile,"role"));auto total=quotaUsage.value("total",json());auto usage=total.is_number()?L"남은 생성 · "+std::to_wstring(quotaUsage.value("remaining",int64_t(0)))+L" / "+std::to_wstring(total.get<int64_t>())+L"장":std::wstring(L"사용 한도 · 무제한");
  float x=c.x+30;for(auto* text:{&role,&usage}){float w=std::min(c.x+c.w-18-x,sTextW(*text,11)+26);if(w<60)break;Rect chip{x,Y(l.chipsY),w,26};renderer.box(chip,0x232327,0x34343b,13);renderer.text(sFit(*text,11,w-18),{chip.x+13,chip.y,chip.w-18,chip.h},0xd0d0d8,11,false,false);x+=w+8;}}
 // 사진 상태 메시지
 if(media){auto note=accountMediaMessage;if(!note.empty())label(note,{c.x+30,Y(l.mediaY),std::max(40.f,c.w-60-(accountMediaRetry.empty()?0.f:80.f)),22},false,0xb0b0b7,11);if(!accountMediaRetry.empty())action(l.mediaY-6,c.x+c.w-18-72,72,L"재시도",[this]{startAccountMedia(accountMediaRetry);},ready);}
 // 프로필 편집
 card(l.cardY,338);label(L"프로필 편집",{c.x+30,Y(l.cardY+12),c.w-60,26},true,0xf2f2f4,14);
 label(L"닉네임",{c.x+30,Y(l.cardY+44),c.w-60,22},false,0x9a9aa6,11);renderer.box({c.x+30,Y(l.nickBoxY),c.w-60,40},0x141416,0x333338,8);
 label(L"자기소개",{c.x+30,Y(l.cardY+118),c.w-60,22},false,0x9a9aa6,11);renderer.box({c.x+30,Y(l.bioBoxY),c.w-60,112},0x141416,0x333338,8);
 label(accountSaveMessage,{c.x+30,Y(l.statusY),c.w-60,22},false,accountSaveFailed?0xf0b3b3:0xa0a0a8,11);float half=(c.w-72)/2;
 action(l.buttonsY,c.x+30,half,accountSaveFailed?L"다시 시도":L"저장",[this]{accountProfileEdited();sendAccountProfile(false,true);},accountDraftDirty&&!accountSaving&&!accountSaveWorkerActive);
 action(l.buttonsY,c.x+38+half,half,L"다시 불러오기",[this]{if(accountDraftDirty&&MessageBoxW(hwnd,L"현재 입력은 초안으로 보관하고 서버 프로필을 다시 불러올까요?",L"프로필",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;sendAccountProfile(true,true);},!accountSaving&&!accountSaveWorkerActive);
 // 계정
 card(l.logoutY,132);label(L"계정",{c.x+30,Y(l.logoutY+12),c.w-60,26},true,0xf2f2f4,14);bool idle=!accountWorkspaceBusy();
 action(l.logoutY+42,c.x+30,c.w-60,L"로그아웃",[this]{featureRequest("account_logout","POST",accountLogoutPath(false));},idle);
 action(l.logoutY+84,c.x+30,c.w-60,L"모든 기기에서 로그아웃",[this]{if(MessageBoxW(hwnd,L"모든 기기의 로그인 세션을 종료할까요?",L"로그아웃",MB_YESNO|MB_ICONQUESTION)==IDYES)featureRequest("account_logout","POST",accountLogoutPath(true));},idle);
 renderer.target->PopAxisAlignedClip();if(accountScrollMax()>0){float thumb=area.h*area.h/(area.h+accountScrollMax());scrollThumb({area.x+area.w-12,area.y,12,area.h},area.y+(area.h-thumb)*accountScroll/accountScrollMax(),thumb,3);}
}
