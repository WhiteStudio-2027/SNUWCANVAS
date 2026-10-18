#pragma once
#include "common.hpp"
#include <windowsx.h>
namespace snuw {
inline void drawDarkChoice(HDC dc,RECT rect,const std::wstring& text,HFONT font,bool hot,bool focus,bool open,bool hardcore=false){
 auto background=CreateSolidBrush(RGB(34,34,36));FillRect(dc,&rect,background);DeleteObject(background);
 auto fill=CreateSolidBrush(hot||open?RGB(58,58,64):RGB(46,46,50));auto pen=CreatePen(PS_SOLID,1,hardcore?RGB(137,73,77):focus||open?RGB(181,194,255):hot?RGB(101,101,115):RGB(67,67,74));auto oldBrush=SelectObject(dc,fill),oldPen=SelectObject(dc,pen);RoundRect(dc,rect.left,rect.top,rect.right,rect.bottom,7,7);SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(fill);DeleteObject(pen);
 auto oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(229,229,231));RECT label=rect;int height=rect.bottom-rect.top;label.left+=std::max(8,height/3);label.right-=height;DrawTextW(dc,text.c_str(),int(text.size()),&label,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(dc,oldFont);
 int x=rect.right-height/2,y=(rect.top+rect.bottom)/2;auto arrow=CreatePen(PS_SOLID,2,RGB(162,162,174));oldPen=SelectObject(dc,arrow);MoveToEx(dc,x-4,y-2,nullptr);LineTo(dc,x,y+2);LineTo(dc,x+4,y-2);SelectObject(dc,oldPen);DeleteObject(arrow);
}
// Editable choices may select their whole label again on WM_SETTEXT/focus.
inline LRESULT CALLBACK editableChoiceProc(HWND h,UINT m,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR){
 if(m==WM_SETTEXT||m==WM_SETFOCUS||m==CB_SETCURSEL){auto result=DefSubclassProc(h,m,a,b);SendMessageW(h,CB_SETEDITSEL,0,MAKELPARAM(-1,0));return result;}
 if(m==WM_NCDESTROY)RemoveWindowSubclass(h,editableChoiceProc,id);
 return DefSubclassProc(h,m,a,b);
}
inline LRESULT CALLBACK darkChoiceProc(HWND h,UINT m,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR){
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_NCPAINT)return 0;
 if(m==WM_PAINT||m==WM_PRINTCLIENT){PAINTSTRUCT ps{};HDC dc=m==WM_PAINT?BeginPaint(h,&ps):(HDC)a;RECT rect;GetClientRect(h,&rect);auto index=SendMessageW(h,CB_GETCURSEL,0,0);std::wstring text;if(index!=CB_ERR){auto length=SendMessageW(h,CB_GETLBTEXTLEN,index,0);if(length>=0){text.resize(size_t(length)+1);SendMessageW(h,CB_GETLBTEXT,index,(LPARAM)text.data());text.resize(size_t(length));}}auto divider=text.find(L" | ");if(divider==32)text=text.substr(35);std::map<std::wstring,std::wstring> labels={{L"prompt",L"생성용 (긍정·부정)"},{L"training",L"학습용 캡션"},{L"tags",L"태그 (쉼표 구분)"},{L"sentence",L"문장"},{L"character",L"캐릭터"},{L"product",L"제품"},{L"brand",L"브랜드"},{L"style",L"스타일"},{L"sdxl",L"SDXL"},{L"illustrious",L"Illustrious XL"}};if(labels.count(text))text=labels[text];drawDarkChoice(dc,rect,text,(HFONT)SendMessageW(h,WM_GETFONT,0,0),GetPropW(h,L"SnuwChoiceHot")!=nullptr,GetFocus()==h,SendMessageW(h,CB_GETDROPPEDSTATE,0,0)!=0,GetPropW(GetParent(h),L"SnuwHardcore")!=nullptr);if(m==WM_PAINT)EndPaint(h,&ps);return 0;}
 if(m==WM_MOUSEMOVE&&!GetPropW(h,L"SnuwChoiceHot")){SetPropW(h,L"SnuwChoiceHot",(HANDLE)1);TRACKMOUSEEVENT track{sizeof track,TME_LEAVE,h,0};TrackMouseEvent(&track);InvalidateRect(h,nullptr,FALSE);}
 if(m==WM_MOUSELEAVE){RemovePropW(h,L"SnuwChoiceHot");InvalidateRect(h,nullptr,FALSE);}
 if(m==WM_SETFOCUS||m==WM_KILLFOCUS||m==WM_LBUTTONUP||m==WM_KEYUP)InvalidateRect(h,nullptr,FALSE);
 if(m==WM_NCDESTROY){RemovePropW(h,L"SnuwChoiceHot");RemoveWindowSubclass(h,darkChoiceProc,id);}return DefSubclassProc(h,m,a,b);
}
}
