#pragma once
// 서버 동기화: 채널·폴더·프롬프트(작업 공간 문서)와 서버 보관함 이미지. 네트워크 작업은 모두 작업 스레드에서 실행하고
// Store 에는 UI 스레드만 접근한다.
#include "api.hpp"
#include "output_layout.hpp"
namespace snuw {
inline bool syncSecretKey(const std::string& key){std::string lower=key;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});return lower.find("api_key")!=std::string::npos||lower.find("password")!=std::string::npos||lower.find("token")!=std::string::npos||lower.find("secret")!=std::string::npos;}
inline void syncStripSecrets(json& value){if(value.is_object()){for(auto it=value.begin();it!=value.end();){if(syncSecretKey(it.key()))it=value.erase(it);else{syncStripSecrets(it.value());++it;}}}else if(value.is_array())for(auto& item:value)syncStripSecrets(item);}
inline const std::set<std::string>& syncDeviceSettingKeys(){static const std::set<std::string> keys={"output","_local_workspace_root","_local_account_scope","api","caption_api","channel_id","channel_name","creator_name","folder_name","initial_image","caption_image","editing_image","editing_session","editing_job","editing_request","_workspace_layout"};return keys;}
inline const std::set<std::string>& syncDeviceStateKeys(){static const std::set<std::string> keys={"selected","geometry","window_state","_gallery_migrations","_story_context","main_tab","prompt_tab","search"};return keys;}
// 다른 기기로 보내도 되는 채널 내용만 남긴다 (기기 고유 경로·선택·창 위치·비밀값 제외).
inline json syncSanitizeChannel(const json& channel){json out={{"id",str(channel,"id")},{"name",str(channel,"name")},{"pinned",flag(channel,"pinned")},{"folder_id",str(channel,"folder_id")}};if(channel.contains("storyboard"))out["storyboard"]=channel["storyboard"];json state=channel.value("state",json::object());for(auto& key:syncDeviceStateKeys())state.erase(key);if(state.contains("settings")&&state["settings"].is_object())for(auto& key:syncDeviceSettingKeys())state["settings"].erase(key);syncStripSecrets(state);out["state"]=std::move(state);return out;}
inline std::string syncChannelHash(const json& sanitized){return sha256(sanitized.dump());}
inline std::string syncFoldersHash(const json& folders){return sha256(folders.dump());}
// 3-way 병합. state.synced(마지막 합의 해시), state.hidden(이 기기에서 지운 채널)을 기준으로 데이터를 잃지 않게 합친다.
inline json mergeWorkspace(const json& local,const json& remote,const json& state){
 json out={{"push",false},{"apply",json::array()},{"backups",json::array()},{"applyFolders",false}};
 std::map<std::string,json> L,R;std::vector<std::string> order;std::set<std::string> hidden;
 for(auto& h:state.value("hidden",json::array()))if(h.is_string())hidden.insert(h.get<std::string>());
 auto base=state.value("synced",json::object());bool hasRemote=remote.is_object()&&remote.contains("channels");
 for(auto& c:local.value("channels",json::array())){auto id=str(c,"id");if(id.empty()||L.count(id))continue;L[id]=c;order.push_back(id);}
 if(hasRemote)for(auto& c:remote.value("channels",json::array())){auto id=str(c,"id");if(id.empty()||R.count(id))continue;R[id]=c;if(!L.count(id))order.push_back(id);}
 json merged=json::array(),synced=json::object();
 for(auto& id:order){bool hasL=L.count(id)>0,hasR=R.count(id)>0;
  if(hasL&&!hasR){merged.push_back(L[id]);out["push"]=true;synced[id]=str(L[id],"hash");continue;}
  if(!hasL&&hasR){merged.push_back(R[id]);synced[id]=str(R[id],"hash");if(!hidden.count(id))out["apply"].push_back(R[id]);continue;}
  auto& l=L[id];auto& r=R[id];auto lh=str(l,"hash"),rh=str(r,"hash"),b=str(base,id);
  if(lh==rh){merged.push_back(l);synced[id]=lh;continue;}
  if(!b.empty()&&lh==b){merged.push_back(r);out["apply"].push_back(r);synced[id]=rh;continue;}
  if(!b.empty()&&rh==b){merged.push_back(l);out["push"]=true;synced[id]=lh;continue;}
  bool localWins=l.value("modified",0.0)>=r.value("modified",0.0);
  if(localWins){merged.push_back(l);out["backups"].push_back(r);out["push"]=true;synced[id]=lh;}
  else{merged.push_back(r);out["backups"].push_back(l);out["apply"].push_back(r);synced[id]=rh;}}
 json folders=local.value("folders",json::array());auto lf=str(local,"foldersHash"),rf=hasRemote?str(remote,"foldersHash"):"",bf=str(state,"foldersSynced");
 if(!hasRemote||rf.empty()){out["push"]=true;}
 else if(lf==rf){}
 else if(!bf.empty()&&lf==bf){folders=remote.value("folders",json::array());out["applyFolders"]=true;}
 else if(!bf.empty()&&rf==bf){out["push"]=true;}
 else{std::set<std::string> have;for(auto& f:folders)have.insert(str(f,"id"));for(auto& f:remote.value("folders",json::array()))if(!have.count(str(f,"id")))folders.push_back(f);out["applyFolders"]=true;out["push"]=true;}
 out["folders"]=folders;out["foldersHash"]=syncFoldersHash(folders);out["synced"]=synced;
 out["merged"]={{"schema",1},{"folders",folders},{"foldersHash",out["foldersHash"]},{"channels",merged}};return out;}
inline json syncRun(const std::string& api,const std::string& key,const json& local,const json& state,const std::atomic_bool* stop){
 json result={{"ok",false}};
 try{
  auto status=request("GET",joinUrl(api,"/sync/status"),key,"","application/json",stop,1<<20,20000);
  if(status.code==404||status.code==405){result["unsupported"]=true;result["error"]="server_without_sync";return result;}
  if(status.code>=300){result["error"]="status_"+std::to_string(status.code);return result;}
  auto info=status.object();result["reset"]=info.value("reset",json::object());
  if(!flag(local,"sync_enabled")){result["ok"]=true;result["statusOnly"]=true;return result;}
  json remote=nullptr;long long revision=info.value("revision",0LL);
  auto fetch=[&]{auto got=request("GET",joinUrl(api,"/sync/workspace"),key,"","application/json",stop,64<<20,60000);if(got.code==404){remote=nullptr;revision=0;return true;}if(got.code>=300)return false;auto body=got.object();remote=body.value("doc",json::object());revision=body.value("revision",0LL);return true;};
  if(flag(info,"enabled")&&!fetch()){result["error"]="download_failed";return result;}
  for(int attempt=0;attempt<4;attempt++){
   auto plan=mergeWorkspace(local,remote,state);
   if(flag(plan,"push")){auto put=request("PUT",joinUrl(api,"/sync/workspace"),key,json{{"expected_revision",revision},{"doc",plan["merged"]}}.dump(),"application/json",stop,1<<20,120000);
    if(put.code==409){if(!fetch()){result["error"]="download_failed";return result;}continue;}
    if(put.code>=300){result["error"]="upload_"+std::to_string(put.code);return result;}
    revision=put.object().value("revision",revision+1);result["pushed"]=true;}
   plan["revision"]=revision;plan["ok"]=true;plan["reset"]=result["reset"];if(!plan.contains("pushed"))plan["pushed"]=result.value("pushed",false);return plan;}
  result["error"]="conflict_retry";return result;
 }catch(const std::exception& e){result["error"]=e.what();return result;}}
// ---- 서버 보관함 이미지 가져오기
struct LibraryProgress{std::atomic<int> total{0},done{0},skipped{0},expired{0},failed{0};std::atomic_bool running{false};std::mutex mutex;std::string message;};
inline std::set<std::string> libraryIndexLoad(const fs::path& workspaceRoot){std::set<std::string> ids;std::ifstream in(workspaceRoot/L"config"/L"library_index.txt",std::ios::binary);std::string line;while(std::getline(in,line)){auto tab=line.find('\t');ids.insert(line.substr(0,tab));}return ids;}
inline std::string syncUrlEncode(const std::string& text){static const char* hex="0123456789ABCDEF";std::string out;for(unsigned char c:text){if(std::isalnum(c)||c=='-'||c=='_'||c=='.'||c=='~')out.push_back(char(c));else{out.push_back('%');out.push_back(hex[c>>4]);out.push_back(hex[c&15]);}}return out;}
inline std::string libraryNormalizedMode(const std::string& mode){return mode=="hardcore"?"hardcore":"standard";}
// 서버 보관함에서 내 이미지 중 로컬에 없는 것을 내려받는다. job: api,key,scope,root,channels{id:{name,settings}}
inline void libraryImportRun(const json& job,std::shared_ptr<LibraryProgress> progress,const std::atomic_bool* stop){
 try{
  auto api=str(job,"api"),key=str(job,"key"),scope=str(job,"scope");fs::path root=fs::path(wide(str(job,"root")));if(!storageId(scope,64))throw std::runtime_error("로그인한 계정에서만 가져올 수 있습니다.");
  auto known=libraryIndexLoad(root);
  auto summary=request("GET",joinUrl(api,"/library/summary"),key,"","application/json",stop,8<<20,30000);if(summary.code==404||summary.code==405)throw std::runtime_error("서버가 보관함 가져오기를 지원하지 않습니다.");if(summary.code>=300)throw std::runtime_error("보관함 요약을 불러오지 못했습니다.");
  int total=summary.object().value("total",0);progress->total=total;
  if(int(known.size())>=total&&total>=0){progress->done=0;return;}
  std::string cursor;auto channels=job.value("channels",json::object());
  while(!(stop&&*stop)){
   std::string path="/library/images?limit=100"+(cursor.empty()?std::string():"&after="+syncUrlEncode(cursor));
   auto page=request("GET",joinUrl(api,path),key,"","application/json",stop,64<<20,60000);if(page.code>=300)throw std::runtime_error("보관함 목록을 불러오지 못했습니다 ("+std::to_string(page.code)+")");
   auto body=page.object();auto images=body.value("images",json::array());
   for(auto& row:images){if(stop&&*stop)break;auto asset=str(row,"asset_id");if(asset.empty()||known.count(asset)){progress->skipped++;continue;}
    try{
     auto channelId=str(row,"channel_id");auto mode=libraryNormalizedMode(str(row,"content_mode"));json cfg=json::object();
     if(channels.contains(channelId)&&channels[channelId].is_object())cfg=channels[channelId].value("settings",json::object());
     if(!storageId(channelId,32)){channelId=std::string(32,'0');}
     if(str(cfg,"output").empty())cfg["output"]=utf8((root/L"generated"/wide(channelId)).wstring());
     cfg["_local_account_scope"]=scope;cfg["_local_workspace_root"]=utf8(root.wstring());cfg["channel_id"]=channelId;cfg["channel_name"]=str(row,"channel_name");cfg["content_mode"]=mode;
     auto image=request("GET",joinUrl(api,str(row,"image_url")),key,"","application/json",stop,256<<20,120000);
     if(image.code==410){progress->expired++;continue;}
     if(image.code>=300){progress->failed++;continue;}
     auto outputRoot=fs::path(wide(str(cfg,"output")));atomicWrite(outputRoot/".snuw-output.json","{\"schema\":1}");
     auto target=outputDirectory(cfg)/("srv_"+timestamp()+"_"+asset.substr(0,8)+".png");if(!localOutputAllowed(cfg,target))throw std::runtime_error("저장할 수 없는 위치입니다.");
     saveImage(target,image.data);registerLocalImage(cfg,target);
     json meta=row.value("metadata",json::object());json settings=meta.value("settings",json::object());settings["content_mode"]=mode;syncStripSecrets(settings);
     json data={{"settings",settings},{"negative",meta.value("negative","")},{"drafts",{{"prompt",meta.value("positive","")}}},{"result",{{"asset_id",asset},{"created",row.value("created",0.0)},{"imported_from_server",true}}}};
     atomicWrite(imageMetadataPath(target),data.dump(2));libraryIndexAppend(root,asset,target);known.insert(asset);progress->done++;
    }catch(const std::exception&){progress->failed++;}
   }
   if(body.contains("next_cursor")&&body["next_cursor"].is_string()&&!body["next_cursor"].get<std::string>().empty())cursor=body["next_cursor"].get<std::string>();else break;}
 }catch(const std::exception& e){std::lock_guard lock(progress->mutex);progress->message=e.what();}
}
}
