#pragma once
#include "server_sync.hpp"
namespace snuw {
// 병합 규칙과 전송 내용 정리(비밀값·기기 고유 값 제외)를 검증한다.
inline int syncMergeTest(const fs::path& folder){
 try{
  fs::create_directories(folder);
  auto channel=[&](const std::string& id,const std::string& name,const std::string& prompt,double modified,const std::string& output="C:/a"){json c={{"id",id},{"name",name},{"pinned",false},{"folder_id",""},{"state",{{"drafts",{{"prompt",prompt}}},{"settings",{{"output",output},{"api_key","SECRET"},{"steps","26"}}},{"selected","C:/x.png"},{"geometry","1x1"},{"main_tab",3}}}};auto s=syncSanitizeChannel(c);s["hash"]=syncChannelHash(s);s["modified"]=modified;return s;};
  auto doc=[&](std::vector<json> channels,json folders=json::array()){json list=json::array();for(auto& c:channels)list.push_back(c);return json{{"folders",folders},{"foldersHash",syncFoldersHash(folders)},{"channels",list}};};
  std::string A(32,'a'),B(32,'b'),C(32,'c');
  // 1) 비밀값·기기 값 제외, 기기 값이 달라도 해시는 같다
  auto one=channel(A,"하나","p",1,"C:/a"),two=channel(A,"하나","p",1,"D:/other");
  if(one.dump().find("SECRET")!=std::string::npos||one["state"].contains("selected")||one["state"]["settings"].contains("output"))throw std::runtime_error("sanitize leaked device or secret values");
  if(str(one,"hash")!=str(two,"hash"))throw std::runtime_error("hash depends on device values");
  // 2) 서버에 문서가 없으면 올린다
  auto r=mergeWorkspace(doc({one}),json(nullptr),json::object());
  if(!flag(r,"push")||!r["apply"].empty())throw std::runtime_error("first push expected");
  // 3) 서버에만 있는 채널은 내려받고, 이 기기에서 지운 채널은 되살리지 않는다
  auto remoteOnly=channel(B,"서버","q",5);
  r=mergeWorkspace(doc({one}),doc({one,remoteOnly}),json::object());
  if(r["apply"].size()!=1||str(r["apply"][0],"id")!=B)throw std::runtime_error("remote-only channel not applied");
  r=mergeWorkspace(doc({one}),doc({one,remoteOnly}),json{{"hidden",json::array({B})}});
  if(!r["apply"].empty()||r["merged"]["channels"].size()!=2)throw std::runtime_error("hidden channel resurrected or dropped from server doc");
  // 4) 한쪽만 바뀌면 바뀐 쪽을 따른다
  auto base=one;auto localChanged=channel(A,"하나","수정",9),remoteChanged=channel(A,"하나","원격 수정",9);
  json state={{"synced",{{A,str(base,"hash")}}}};
  r=mergeWorkspace(doc({localChanged}),doc({base}),state);
  if(!flag(r,"push")||!r["apply"].empty()||!r["backups"].empty())throw std::runtime_error("local-only change should push");
  r=mergeWorkspace(doc({base}),doc({remoteChanged}),state);
  if(flag(r,"push")||r["apply"].size()!=1||!r["backups"].empty())throw std::runtime_error("remote-only change should apply");
  // 5) 양쪽이 바뀌면 최신 수정이 이기고 진 쪽은 백업으로 남는다
  auto localNewer=channel(A,"하나","로컬 최신",20),remoteOlder=channel(A,"하나","원격 이전",10);
  r=mergeWorkspace(doc({localNewer}),doc({remoteOlder}),state);
  if(!flag(r,"push")||r["backups"].size()!=1||str(r["backups"][0]["state"]["drafts"],"prompt")!="원격 이전")throw std::runtime_error("conflict should keep local and back up remote");
  auto localOlder=channel(A,"하나","로컬 이전",10),remoteNewer=channel(A,"하나","원격 최신",20);
  r=mergeWorkspace(doc({localOlder}),doc({remoteNewer}),state);
  if(r["apply"].size()!=1||r["backups"].size()!=1||str(r["backups"][0]["state"]["drafts"],"prompt")!="로컬 이전")throw std::runtime_error("conflict should apply remote and back up local");
  // 6) 폴더 충돌은 합집합으로 합친다
  json lf=json::array({json{{"id","f1"},{"name","로컬"},{"collapsed",false}}}),rf=json::array({json{{"id","f2"},{"name","원격"},{"collapsed",false}}});
  r=mergeWorkspace(doc({one},lf),doc({one},rf),json{{"foldersSynced","old"}});
  if(r["folders"].size()!=2||!flag(r,"applyFolders")||!flag(r,"push"))throw std::runtime_error("folder union failed");
  // 7) 같은 내용이면 아무것도 하지 않는다
  r=mergeWorkspace(doc({one}),doc({one}),state);
  if(flag(r,"push")||!r["apply"].empty()||!r["backups"].empty())throw std::runtime_error("identical docs should be a no-op");
  atomicWrite(folder/"sync-result.json",json{{"ok",true},{"cases",7}}.dump(2));return 0;
 }catch(const std::exception& e){atomicWrite(folder/"sync-result.json",json{{"ok",false},{"error",e.what()}}.dump(2));return 1;}}
}
