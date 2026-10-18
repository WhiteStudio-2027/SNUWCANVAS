#pragma once
#include "common.hpp"
#include <richedit.h>
#include <richole.h>
#include <tom.h>
#include <windowsx.h>
#include <regex>
#include "prompt_tags.hpp"
namespace snuw {
// Standard edit controls do not implement Ctrl+A themselves. Keep clipboard,
// undo, navigation and IME handling in Windows; handle selection consistently.
inline LRESULT CALLBACK textShortcutProc(HWND h,UINT m,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR){
 if(m==WM_KEYDOWN&&(GetKeyState(VK_CONTROL)&0x8000)&&a=='A'){SendMessageW(h,EM_SETSEL,0,-1);return 0;}
 if(m==WM_CHAR&&a==1)return 0;
 if(m==WM_NCDESTROY)RemoveWindowSubclass(h,textShortcutProc,id);
 return DefSubclassProc(h,m,a,b);
}
inline LRESULT CALLBACK checkPaintProc(HWND h,UINT m,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR){
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT||m==WM_PRINTCLIENT){PAINTSTRUCT ps{};auto dc=m==WM_PAINT?BeginPaint(h,&ps):(HDC)a;RECT r;GetClientRect(h,&r);auto bg=CreateSolidBrush(RGB(27,27,28));FillRect(dc,&r,bg);DeleteObject(bg);int size=std::min(r.right-r.left,r.bottom-r.top)-4;RECT box{2,(r.bottom-size)/2,2+size,(r.bottom+size)/2};bool checked=SendMessageW(h,BM_GETCHECK,0,0)==BST_CHECKED;auto fill=CreateSolidBrush(checked?RGB(181,194,255):RGB(37,38,42));FillRect(dc,&box,fill);DeleteObject(fill);auto border=CreateSolidBrush(GetPropW(GetParent(h),L"SnuwHardcore")?RGB(137,73,77):GetFocus()==h?RGB(210,218,255):RGB(106,108,120));FrameRect(dc,&box,border);DeleteObject(border);if(checked){auto pen=CreatePen(PS_SOLID,std::max(2,size/8),RGB(25,29,39));auto old=SelectObject(dc,pen);MoveToEx(dc,box.left+size/4,box.top+size/2,nullptr);LineTo(dc,box.left+size*4/9,box.top+size*3/4);LineTo(dc,box.left+size*4/5,box.top+size/4);SelectObject(dc,old);DeleteObject(pen);}if(m==WM_PAINT)EndPaint(h,&ps);return 0;}
 auto result=DefSubclassProc(h,m,a,b);
 if(m==BM_SETCHECK||m==WM_LBUTTONUP||m==WM_KEYUP||m==WM_SETFOCUS||m==WM_KILLFOCUS)InvalidateRect(h,nullptr,FALSE);
 if(m==WM_NCDESTROY)RemoveWindowSubclass(h,checkPaintProc,id);return result;
}
inline std::string normalizedText(HWND h){auto s=utf8(textOf(h));std::string out;for(size_t i=0;i<s.size();i++){if(s[i]=='\r'){out+='\n';if(i+1<s.size()&&s[i+1]=='\n')i++;}else out+=s[i];}return out;}
inline void highlightEditor(HWND h,bool negative){
 if(!h)return;Com<IRichEditOle> ole;SendMessageW(h,EM_GETOLEINTERFACE,0,(LPARAM)ole.GetAddressOf());Com<ITextDocument> document;GUID iid;IIDFromString(L"{8CC497C0-A1DF-11CE-8098-00AA0047BE5D}",&iid);if(!ole||FAILED(ole->QueryInterface(iid,reinterpret_cast<void**>(document.GetAddressOf()))))return;long undone=0;document->Undo(tomSuspend,&undone);CHARRANGE selected;SendMessageW(h,EM_EXGETSEL,0,(LPARAM)&selected);POINT scroll;SendMessageW(h,EM_GETSCROLLPOS,0,(LPARAM)&scroll);auto mask=SendMessageW(h,EM_GETEVENTMASK,0,0);SendMessageW(h,EM_SETEVENTMASK,0,0);SendMessageW(h,WM_SETREDRAW,FALSE,0);
 // One consistent foreground avoids splitting shaped glyphs and CR/LF offset mismatches.
 CHARFORMAT2W format{};format.cbSize=sizeof format;format.dwMask=CFM_COLOR;format.crTextColor=negative?RGB(215,177,177):RGB(229,229,231);
 SendMessageW(h,EM_SETCHARFORMAT,SCF_ALL,(LPARAM)&format);
 SendMessageW(h,EM_EXSETSEL,0,(LPARAM)&selected);SendMessageW(h,EM_SETSCROLLPOS,0,(LPARAM)&scroll);SendMessageW(h,EM_SETEVENTMASK,0,mask);SendMessageW(h,WM_SETREDRAW,TRUE,0);document->Undo(tomResume,&undone);InvalidateRect(h,nullptr,FALSE);
}
inline void adjustTagWeight(HWND h,int direction){
 GETTEXTLENGTHEX length{GTL_NUMCHARS|GTL_PRECISE,1200};auto count=SendMessageW(h,EM_GETTEXTLENGTHEX,(WPARAM)&length,0);
 std::wstring source(size_t(count)+1,L'\0');GETTEXTEX get{DWORD(source.size()*sizeof(wchar_t)),GT_DEFAULT,1200,nullptr,nullptr};auto read=SendMessageW(h,EM_GETTEXTEX,(WPARAM)&get,(LPARAM)source.data());source.resize(size_t(read));CHARRANGE selected;SendMessageW(h,EM_EXGETSEL,0,(LPARAM)&selected);
 auto tag=weightedTag(source,size_t(selected.cpMin),direction);if(!tag.valid)return;CHARRANGE range{LONG(tag.begin),LONG(tag.end)};SendMessageW(h,EM_EXSETSEL,0,(LPARAM)&range);SendMessageW(h,EM_REPLACESEL,TRUE,(LPARAM)tag.text.c_str());range={LONG(tag.begin),LONG(tag.begin+tag.text.size())};SendMessageW(h,EM_EXSETSEL,0,(LPARAM)&range);
}
inline LRESULT CALLBACK promptProc(HWND h,UINT m,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR){
 if(m==WM_KEYDOWN&&(GetKeyState(VK_CONTROL)&0x8000)&&(a==VK_UP||a==VK_DOWN)){adjustTagWeight(h,a==VK_UP?1:-1);return 0;}
 if(m==WM_CONTEXTMENU){POINT p{GET_X_LPARAM(b),GET_Y_LPARAM(b)};if(p.x==-1){GetCaretPos(&p);ClientToScreen(h,&p);}else{POINT local=p;ScreenToClient(h,&local);CHARRANGE range;SendMessageW(h,EM_EXGETSEL,0,(LPARAM)&range);if(range.cpMin==range.cpMax){auto index=SendMessageW(h,EM_CHARFROMPOS,0,(LPARAM)&local);range={LONG(index),LONG(index)};SendMessageW(h,EM_EXSETSEL,0,(LPARAM)&range);}}HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,L"태그 가중치 +0.1\tCtrl+↑");AppendMenuW(menu,MF_STRING,2,L"태그 가중치 −0.1\tCtrl+↓");AppendMenuW(menu,MF_STRING,3,L"태그 가중치 초기화");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,4,L"실행 취소");AppendMenuW(menu,MF_STRING,5,L"잘라내기");AppendMenuW(menu,MF_STRING,6,L"복사");AppendMenuW(menu,MF_STRING,7,L"붙여넣기");AppendMenuW(menu,MF_STRING,8,L"모두 선택");int cmd=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,h,nullptr);DestroyMenu(menu);if(cmd>=1&&cmd<=3)adjustTagWeight(h,cmd==1?1:cmd==2?-1:0);else if(cmd==4)SendMessageW(h,EM_UNDO,0,0);else if(cmd>=5&&cmd<=7)SendMessageW(h,cmd==5?WM_CUT:cmd==6?WM_COPY:WM_PASTE,0,0);else if(cmd==8){CHARRANGE all{0,-1};SendMessageW(h,EM_EXSETSEL,0,(LPARAM)&all);}return 0;}
 if(m==WM_NCDESTROY)RemoveWindowSubclass(h,promptProc,id);return DefSubclassProc(h,m,a,b);
}

}
