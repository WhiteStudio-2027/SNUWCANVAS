#pragma once
// 계정 모드의 저장 폴더는 ID 로 되어 있어 찾기 어렵다. 실제 저장 위치와 소유 표시는 그대로 두고,
// 채널 이름으로 된 폴더 바로가기(NTFS 정션)를 "채널별 보기" 폴더에 만들어 유지한다. 데이터는 옮기지 않는다.
#include "common.hpp"
#include "output_layout.hpp"
#include "model.hpp"
#include <winioctl.h>
namespace snuw {
inline bool isReparseDirectory(const std::wstring& path){auto attributes=GetFileAttributesW(path.c_str());return attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_DIRECTORY)&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT);}
inline std::wstring plainAbsolute(const fs::path& path){auto value=extendedWindowsPath(path);if(value.rfind(L"\\\\?\\UNC\\",0)==0)return L"\\\\"+value.substr(8);if(value.rfind(L"\\\\?\\",0)==0)return value.substr(4);return value;}
inline bool createJunction(const fs::path& link,const fs::path& target){
 auto linkPath=extendedWindowsPath(link);if(!CreateDirectoryW(linkPath.c_str(),nullptr))return false;
 HANDLE handle=CreateFileW(linkPath.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
 if(handle==INVALID_HANDLE_VALUE){RemoveDirectoryW(linkPath.c_str());return false;}
 auto printName=plainAbsolute(target);while(printName.size()>3&&printName.back()==L'\\')printName.pop_back();auto substitute=L"\\??\\"+printName;
 USHORT subBytes=USHORT(substitute.size()*2),printBytes=USHORT(printName.size()*2);USHORT dataLength=USHORT(8+subBytes+2+printBytes+2);
 std::vector<BYTE> buffer(8+dataLength,0);DWORD tag=IO_REPARSE_TAG_MOUNT_POINT;memcpy(buffer.data(),&tag,4);memcpy(buffer.data()+4,&dataLength,2);
 USHORT fields[4]={0,subBytes,USHORT(subBytes+2),printBytes};memcpy(buffer.data()+8,fields,8);
 memcpy(buffer.data()+16,substitute.c_str(),subBytes);memcpy(buffer.data()+16+subBytes+2,printName.c_str(),printBytes);
 DWORD returned=0;bool ok=DeviceIoControl(handle,FSCTL_SET_REPARSE_POINT,buffer.data(),DWORD(buffer.size()),nullptr,0,&returned,nullptr)!=FALSE;CloseHandle(handle);
 if(!ok)RemoveDirectoryW(linkPath.c_str());return ok;}
inline const wchar_t* channelShortcutFolderName(){return L"채널별 보기";}
// 채널 이름 → 실제 채널 폴더 바로가기. 실패해도 예외를 던지지 않는다.
inline void syncChannelShortcuts(Store& store){
 static std::map<std::wstring,std::wstring> made;static std::string lastSignature;
 try{
  if(store.workspaceScope.size()!=64)return;auto workspace=str(store.settings,"_local_workspace_root");if(workspace.empty())return;
  auto scope=store.workspaceScope.substr(0,32);fs::path base=fs::path(wide(workspace))/L"generated"/channelShortcutFolderName();
  struct Item{std::wstring name;fs::path target;std::string id,title;};std::vector<Item> items;std::set<std::wstring> used;
  for(auto& channel:store.doc["channels"]){auto id=str(channel,"id");if(!storageId(id,32))continue;auto settings=channel["state"].value("settings",json::object());auto output=str(settings,"output");if(output.empty())continue;
   fs::path target=fs::path(wide(output))/L"data"/wide(scope)/wide(id);std::error_code ec;if(!fs::is_directory(target,ec))continue;
   auto title=str(channel,"name");auto name=outputName(title,L"채널");auto unique=name;for(int n=2;used.count(settingFold(unique));n++)unique=name+L" ("+std::to_wstring(n)+L")";used.insert(settingFold(unique));items.push_back({unique,target,id,title});}
  std::string signature=utf8(base.wstring());for(auto& item:items)signature+="|"+utf8(item.name)+"="+utf8(item.target.wstring());
  bool changed=signature!=lastSignature;
  std::error_code ec;fs::create_directories(base,ec);if(ec)return;
  // 채널 폴더에 이름 표식을 남긴다 (폴더를 직접 옮겨도 어떤 채널인지 알 수 있다).
  for(auto& item:items){json marker={{"channel_id",item.id},{"channel_name",item.title}};auto file=item.target/L".snuw-channel.json";bool same=false;try{if(fs::exists(file))same=readJson(file)==marker;}catch(...){}if(!same)atomicWrite(file,marker.dump(2));}
  if(!changed){bool intact=true;for(auto& item:items)if(!isReparseDirectory(extendedWindowsPath(base/item.name))){intact=false;break;}if(intact)return;}
  // 더 이상 필요 없는 정션만 제거한다 (정션 자체만 지우며 안쪽 파일은 건드리지 않는다).
  std::set<std::wstring> wanted;for(auto& item:items)wanted.insert(settingFold(item.name));
  for(auto& entry:fs::directory_iterator(base,ec)){auto name=entry.path().filename().wstring();auto full=extendedWindowsPath(entry.path());if(!isReparseDirectory(full))continue;auto key=settingFold(name);auto previous=made.find(key);bool stale=!wanted.count(key);if(!stale&&previous!=made.end()){for(auto& item:items)if(settingFold(item.name)==key&&previous->second!=item.target.wstring())stale=true;}if(stale)RemoveDirectoryW(full.c_str());}
  for(auto& item:items){auto link=base/item.name;auto full=extendedWindowsPath(link);if(isReparseDirectory(full)&&made.count(settingFold(item.name))&&made[settingFold(item.name)]==item.target.wstring())continue;if(isReparseDirectory(full))RemoveDirectoryW(full.c_str());if(GetFileAttributesW(full.c_str())!=INVALID_FILE_ATTRIBUTES)continue;if(createJunction(link,item.target))made[settingFold(item.name)]=item.target.wstring();}
  auto note=base/L"README.txt";if(!fs::exists(note,ec)){std::string text="이 폴더의 항목은 채널 이름으로 연결된 바로가기입니다.\r\n실제 이미지는 각 채널의 저장 폴더(ID 이름)에 보관되며, 여기서 열어도 같은 파일입니다.\r\n바로가기 폴더 안의 이미지를 지우면 실제 이미지도 지워집니다.\r\n이 폴더는 앱이 자동으로 관리하므로 직접 이름을 바꾸지 않아도 됩니다.\r\n";std::ofstream out(note,std::ios::binary);out<<text;}
  lastSignature=signature;
 }catch(...){}
}
}
