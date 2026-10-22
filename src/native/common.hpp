#pragma once
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#define WINVER 0x0A00
#define NTDDI_VERSION 0x0A000003
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <winhttp.h>
#include <dwmapi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "third_party/json.hpp"
namespace snuw {
using json=nlohmann::json;
namespace fs=std::filesystem;
template<class T> using Com=Microsoft::WRL::ComPtr<T>;
inline std::string utf8(const std::wstring& v){if(v.empty())return {};int n=WideCharToMultiByte(CP_UTF8,0,v.data(),int(v.size()),nullptr,0,nullptr,nullptr);std::string s(n,0);WideCharToMultiByte(CP_UTF8,0,v.data(),int(v.size()),s.data(),n,nullptr,nullptr);return s;}
inline std::wstring wide(const std::string& v){if(v.empty())return {};int n=MultiByteToWideChar(CP_UTF8,0,v.data(),int(v.size()),nullptr,0);std::wstring s(n,0);MultiByteToWideChar(CP_UTF8,0,v.data(),int(v.size()),s.data(),n);return s;}
inline std::string str(const json& o,const std::string& k,std::string fallback=""){if(!o.is_object()||!o.contains(k)||o[k].is_null())return fallback;if(o[k].is_string())return o[k].get<std::string>();return o[k].dump();}
inline bool flag(const json& o,const std::string& k,bool fallback=false){if(!o.contains(k))return fallback;if(o[k].is_boolean())return bool(o[k]);return str(o,k)=="true";}
inline std::string id(){GUID g;CoCreateGuid(&g);wchar_t b[40];StringFromGUID2(g,b,40);std::string s;for(auto c:std::wstring(b))if(iswxdigit(c))s+=char(towlower(c));return s;}
inline std::string readFile(const fs::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("파일 읽기 실패: "+utf8(p.wstring()));return std::string(std::istreambuf_iterator<char>(f),{});}
inline json readJson(const fs::path& p){auto s=readFile(p);return json::parse(s);}
inline void atomicWrite(const fs::path& p,const std::string& s){fs::create_directories(p.parent_path());auto tmp=p;tmp+=L"."+wide(id())+L".tmp";{std::ofstream f(tmp,std::ios::binary);f.write(s.data(),s.size());f.flush();if(!f)throw std::runtime_error("저장 실패");}if(!MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){fs::remove(tmp);throw std::runtime_error("파일 교체 실패");}}
inline fs::path exeDir(){wchar_t b[32768];GetModuleFileNameW(nullptr,b,32768);return fs::path(b).parent_path();}
inline std::wstring textOf(HWND h){int n=GetWindowTextLengthW(h);std::wstring s(n+1,0);GetWindowTextW(h,s.data(),n+1);s.resize(n);return s;}
inline std::string timestamp(){SYSTEMTIME t;GetLocalTime(&t);char b[64];snprintf(b,sizeof b,"%04d%02d%02d_%02d%02d%02d_%03d",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);return b;}
inline std::wstring fileDialog(HWND owner,bool save,const wchar_t* filter,const wchar_t* ext=nullptr){wchar_t b[32768]={};OPENFILENAMEW o{sizeof o};o.hwndOwner=owner;o.lpstrFilter=filter;o.lpstrFile=b;o.nMaxFile=32768;o.lpstrDefExt=ext;o.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);return (save?GetSaveFileNameW(&o):GetOpenFileNameW(&o))?b:L"";}
inline std::wstring pickFolder(HWND owner){Com<IFileDialog> d;if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&d))))return {};DWORD f;d->GetOptions(&f);d->SetOptions(f|FOS_PICKFOLDERS|FOS_FORCEFILESYSTEM);if(FAILED(d->Show(owner)))return {};Com<IShellItem> item;d->GetResult(&item);PWSTR p=nullptr;item->GetDisplayName(SIGDN_FILESYSPATH,&p);std::wstring s=p?p:L"";CoTaskMemFree(p);return s;}
inline void openPath(const fs::path& path){ShellExecuteW(nullptr,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}
inline std::string mime(const fs::path& p){auto e=p.extension().wstring();std::transform(e.begin(),e.end(),e.begin(),::towlower);if(e==L".jpg"||e==L".jpeg")return "image/jpeg";if(e==L".webp")return "image/webp";if(e==L".bmp")return "image/bmp";return "image/png";}
inline bool imagePath(const fs::path& p){auto e=p.extension().wstring();std::transform(e.begin(),e.end(),e.begin(),::towlower);return e==L".png"||e==L".jpg"||e==L".jpeg"||e==L".webp"||e==L".bmp"||e==L".gif"||e==L".tif"||e==L".tiff"||e==L".ico";}
struct Rect{float x=0,y=0,w=0,h=0;bool hit(float a,float b)const{return a>=x&&b>=y&&a<x+w&&b<y+h;}D2D1_RECT_F d2d()const{return D2D1::RectF(x,y,x+w,y+h);}};
inline D2D1_COLOR_F color(unsigned rgb,float alpha=1){unsigned r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;if(!(r>g*1.3f&&r>b*1.2f)){unsigned v=unsigned(.2126f*r+.7152f*g+.0722f*b);rgb=(v<<16)|(v<<8)|v;}return D2D1::ColorF(rgb,alpha);}
inline void check(HRESULT hr,const char* message){if(FAILED(hr))throw std::runtime_error(message);}
}
