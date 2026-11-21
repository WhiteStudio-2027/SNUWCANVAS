// 서버 동기화(채널·폴더·프롬프트, 보관함 이미지) UI 연동. App 클래스 본문 안에서 include 된다.
std::mutex syncMutex;std::deque<std::function<void()>> syncQueue;std::atomic_bool syncStop{false};std::atomic<int> syncWorkers{0};
bool syncRunning=false,syncUnsupported=false,syncForce=false,libraryRunning=false,libraryAutoChecked=false;ULONGLONG syncLast=0;fs::path syncStateRoot;
json syncState=json::object();std::wstring syncStatusText=L"대기 중",resetNotice;std::shared_ptr<LibraryProgress> libraryProgress;
fs::path syncStatePath()const{return store->root/L"config"/L"sync_state.json";}
void loadSyncState(){if(syncStateRoot==store->root)return;syncStateRoot=store->root;syncState=json::object();try{if(fs::exists(syncStatePath()))syncState=readJson(syncStatePath());}catch(...){}if(!syncState.is_object())syncState=json::object();}
void saveSyncState(){try{atomicWrite(syncStatePath(),syncState.dump(2));}catch(...){}}
void markChannelHidden(const std::string& channel){loadSyncState();auto hidden=syncState.value("hidden",json::array());if(!hidden.is_array())hidden=json::array();hidden.push_back(channel);syncState["hidden"]=hidden;saveSyncState();}
bool syncAvailable()const{return !welcome&&!closing&&hwnd&&store->workspaceScope.size()==64&&!str(store->settings,"api_key").empty();}
json buildLocalSyncDoc(){loadSyncState();json meta=syncState.value("meta",json::object());if(!meta.is_object())meta=json::object();json channels=json::array();double now=double(std::time(nullptr));std::set<std::string> present;
 for(auto& channel:store->doc["channels"]){auto sanitized=syncSanitizeChannel(channel);auto id=str(sanitized,"id");if(!storageId(id,32))continue;present.insert(id);auto hash=syncChannelHash(sanitized);auto& m=meta[id];if(!m.is_object()||str(m,"hash")!=hash)m={{"hash",hash},{"modified",now}};sanitized["hash"]=hash;sanitized["modified"]=m.value("modified",now);channels.push_back(sanitized);}
 json keep=json::array();for(auto& h:syncState.value("hidden",json::array()))if(h.is_string()&&!present.count(h.get<std::string>()))keep.push_back(h);syncState["hidden"]=keep;syncState["meta"]=meta;
 json folders=json::array();for(auto& f:store->doc.value("folders",json::array()))folders.push_back({{"id",str(f,"id")},{"name",str(f,"name")},{"collapsed",flag(f,"collapsed")}});
 return {{"folders",folders},{"foldersHash",syncFoldersHash(folders)},{"channels",channels},{"sync_enabled",flag(store->settings,"sync_enabled")}};}
void syncTick(){if(!syncAvailable()||syncRunning||syncUnsupported||promptComposing)return;ULONGLONG now=GetTickCount64();bool enabled=flag(store->settings,"sync_enabled");if(!syncForce&&syncLast&&now-syncLast<(enabled?30000u:600000u))return;syncForce=false;syncLast=now;
 auto local=buildLocalSyncDoc();auto stateCopy=syncState;auto api=str(store->settings,"api"),key=str(store->settings,"api_key");syncRunning=true;if(enabled)syncStatusText=L"동기화 중…";syncWorkers++;
 std::thread([this,api,key,local,stateCopy]{auto result=syncRun(api,key,local,stateCopy,&syncStop);{std::lock_guard lock(syncMutex);syncQueue.push_back([this,result]{applySyncResult(result);});}syncWorkers--;}).detach();}
void drainSyncQueue(){std::deque<std::function<void()>> items;{std::lock_guard lock(syncMutex);items.swap(syncQueue);}for(auto& f:items)f();}
void writeSyncBackup(const json& entry){try{auto folder=store->root/L"config"/L"sync-backups";fs::create_directories(folder);atomicWrite(folder/wide(str(entry,"id")+"-"+std::to_string(std::time(nullptr))+".json"),entry.dump(2));}catch(...){}}
bool applyRemoteChannel(const json& entry,bool& activeChanged){auto id=str(entry,"id");if(!storageId(id,32)||!entry.contains("state")||!entry["state"].is_object())return false;json* local=nullptr;for(auto& c:store->doc["channels"])if(str(c,"id")==id)local=&c;json state=entry["state"];
 if(!state.contains("settings")||!state["settings"].is_object())state["settings"]=json::object();
 if(local){json old=local->value("state",json::object());for(auto& k:syncDeviceStateKeys()){if(old.contains(k))state[k]=old[k];else state.erase(k);}auto oldSettings=old.value("settings",json::object());for(auto& k:syncDeviceSettingKeys()){if(oldSettings.contains(k))state["settings"][k]=oldSettings[k];else state["settings"].erase(k);}
  (*local)["name"]=str(entry,"name");(*local)["pinned"]=flag(entry,"pinned");(*local)["folder_id"]=str(entry,"folder_id");if(entry.contains("storyboard"))(*local)["storyboard"]=entry["storyboard"];(*local)["state"]=state;
  if(id==store->active){store->state=(*local)["state"];store->restore();activeChanged=true;}}
 else{state["selected"]=nullptr;state["settings"]["output"]=utf8((store->root/L"generated"/wide(id)).wstring());json channel={{"id",id},{"name",str(entry,"name")},{"pinned",flag(entry,"pinned")},{"folder_id",str(entry,"folder_id")},{"state",state}};if(entry.contains("storyboard"))channel["storyboard"]=entry["storyboard"];store->doc["channels"].push_back(channel);}
 return true;}
void updateResetNotice(const json& reset){std::wstring before=resetNotice;if(!reset.is_object()||!reset.contains("scheduled_at")||!reset["scheduled_at"].is_number()){resetNotice.clear();return;}
 double at=reset["scheduled_at"].get<double>(),server=reset.value("server_time",double(std::time(nullptr))),remain=at-server;time_t when=time_t(double(std::time(nullptr))+remain);tm local{};localtime_s(&local,&when);wchar_t buffer[64]{};wcsftime(buffer,64,L"%Y-%m-%d %H:%M",&local);
 std::wstring text=L"서버가 "+std::wstring(buffer)+L"에 초기화될 예정입니다";if(remain>0){int days=int(remain/86400),hours=int(std::fmod(remain,86400.0)/3600);text+=L" ("+(days?std::to_wstring(days)+L"일 ":std::wstring())+std::to_wstring(hours)+L"시간 후)";}else text+=L" (진행 중)";
 text+=L". 동기화된 채널과 이미지가 사라지니 필요한 이미지는 미리 저장하세요.";auto message=str(reset,"message");if(!message.empty())text+=L" "+wide(message);resetNotice=text;if(resetNotice!=before)settingsToast(L"서버 초기화가 예약되어 있습니다.");}
void applySyncResult(const json& r){syncRunning=false;
 if(flag(r,"unsupported")){syncUnsupported=true;syncStatusText=L"서버가 동기화를 지원하지 않습니다";return;}
 if(!flag(r,"ok")){syncStatusText=L"동기화 실패 · 잠시 후 다시 시도합니다";return;}
 updateResetNotice(r.value("reset",json::object()));if(flag(r,"statusOnly")){syncStatusText=L"동기화 꺼짐";return;}
 loadSyncState();bool applied=false,activeChanged=false;json meta=syncState.value("meta",json::object());if(!meta.is_object())meta=json::object();
 for(auto& backup:r.value("backups",json::array()))writeSyncBackup(backup);
 for(auto& entry:r.value("apply",json::array()))if(applyRemoteChannel(entry,activeChanged)){applied=true;meta[str(entry,"id")]={{"hash",str(entry,"hash")},{"modified",entry.value("modified",0.0)}};}
 if(flag(r,"applyFolders")){auto& folders=store->doc["folders"];if(!folders.is_array())folders=json::array();for(auto& remote:r.value("folders",json::array())){auto id=str(remote,"id");if(id.empty())continue;bool found=false;for(auto& f:folders)if(str(f,"id")==id){f["name"]=str(remote,"name");f["collapsed"]=flag(remote,"collapsed");found=true;}if(!found)folders.push_back({{"id",id},{"name",str(remote,"name")},{"collapsed",flag(remote,"collapsed")}});}applied=true;}
 syncState["meta"]=meta;syncState["revision"]=r.value("revision",0LL);syncState["synced"]=r.value("synced",json::object());syncState["foldersSynced"]=str(r,"foldersHash");saveSyncState();
 if(applied){try{store->save();}catch(...){}if(activeChanged)rebuild(false);scan();}
 wchar_t clock[32]{};{time_t t=std::time(nullptr);tm lt{};localtime_s(&lt,&t);wcsftime(clock,32,L"%H:%M",&lt);}syncStatusText=L"동기화됨 · "+std::wstring(clock);
 if(!libraryAutoChecked||applied){libraryAutoChecked=true;startLibraryImport(false);}
 if(hwnd)InvalidateRect(hwnd,nullptr,FALSE);}
void finishLibraryImport(std::shared_ptr<LibraryProgress> progress,bool manual){std::string message;{std::lock_guard lock(progress->mutex);message=progress->message;}
 if(!message.empty()){if(manual)settingsToast(wide(message));return;}
 int done=progress->done,expired=progress->expired,failed=progress->failed;
 if(done>0){std::wstring text=L"서버에서 이미지 "+std::to_wstring(done)+L"장을 가져왔습니다.";if(expired>0)text+=L" (만료 "+std::to_wstring(expired)+L"장 제외)";settingsToast(text);status=text;scan();}
 else if(manual){std::wstring text=L"새로 가져올 이미지가 없습니다.";if(expired>0)text=L"원본이 만료된 이미지 "+std::to_wstring(expired)+L"장은 가져올 수 없습니다.";if(failed>0)text+=L" (실패 "+std::to_wstring(failed)+L"장)";settingsToast(text);}}
void startLibraryImport(bool manual){if(libraryRunning)return;if(!syncAvailable()){if(manual)settingsToast(L"로그인한 상태에서만 사용할 수 있습니다.");return;}
 json job={{"api",str(store->settings,"api")},{"key",str(store->settings,"api_key")},{"scope",store->workspaceScope},{"root",utf8(store->root.wstring())}};json channels=json::object();for(auto& c:store->doc["channels"]){auto id=str(c,"id");if(id.empty())continue;channels[id]={{"name",str(c,"name")},{"settings",c.value("state",json::object()).value("settings",json::object())}};}job["channels"]=channels;
 auto progress=std::make_shared<LibraryProgress>();libraryProgress=progress;libraryRunning=true;progress->running=true;syncWorkers++;
 std::thread([this,job,progress,manual]{libraryImportRun(job,progress,&syncStop);progress->running=false;{std::lock_guard lock(syncMutex);syncQueue.push_back([this,progress,manual]{libraryRunning=false;finishLibraryImport(progress,manual);});}syncWorkers--;}).detach();
 if(manual)settingsToast(L"서버 이미지 가져오기를 시작했습니다.");}
std::wstring syncStatusDisplay(){if(libraryRunning&&libraryProgress){int total=libraryProgress->total,done=libraryProgress->done+libraryProgress->skipped+libraryProgress->expired+libraryProgress->failed;return L"이미지 확인 중 "+std::to_wstring(std::min(done,total))+L" / "+std::to_wstring(total);}if(!flag(store->settings,"sync_enabled")&&!syncUnsupported)return L"꺼짐";return syncStatusText;}
void paintResetNotice(){if(resetNotice.empty()||welcome)return;float maxW=std::max(200.f,width-120.f);auto text=sFit(L"서버 초기화 예약 · "+resetNotice,12,maxW-28);float w=std::min(maxW,sTextW(text,12)+28);Rect pill{(width-w)/2,3,w,22};renderer.box({pill.x,pill.y+1,pill.w,pill.h},0x0e0e10,0,11);renderer.box(pill,0x3a2a12,0xe0b45c,11);renderer.text(text,pill,0xffd58a,12,false,true);}
void waitForSyncWorkers(){syncStop=true;for(int i=0;i<80&&syncWorkers>0;++i)Sleep(50);}
