// Owned dataset operations; uploads are streamed as individual bounded requests.
std::thread referenceTransferThread,referenceThumbThread;
std::atomic_bool referenceTransferCancel{false},referenceThumbCancel{false},referenceShutdown{false};
bool referenceUploadRunning=false,referencePreviewLoading=false;
std::wstring referencePreviewError;
struct ReferenceDownload {json cfg;fs::path root,target;std::string project,profile,asset;bool preview=false;uint64_t sequence=0;};
std::mutex referenceDownloadMutex;std::condition_variable referenceDownloadWake;
std::deque<ReferenceDownload> referenceDownloads;std::map<fs::path,uint64_t> referencePending;
uint64_t referencePreviewSequence=0;
void cancelReferenceDownloads(){
 std::lock_guard lock(referenceDownloadMutex);referenceDownloads.clear();referencePending.clear();referenceThumbCancel=true;++referencePreviewSequence;referencePreviewLoading=false;
}
void queueReferenceDownload(ReferenceDownload task){
 {std::lock_guard lock(referenceDownloadMutex);
  if(task.preview){for(auto it=referenceDownloads.begin();it!=referenceDownloads.end();)if(it->preview){referencePending.erase(it->target);it=referenceDownloads.erase(it);}else ++it;}
  if(task.preview)referencePending.erase(task.target);if(!referencePending.emplace(task.target,task.sequence).second)return;
  if(task.preview)referenceDownloads.push_front(std::move(task));else referenceDownloads.push_back(std::move(task));
 }
 if(!referenceThumbThread.joinable())referenceThumbThread=std::thread([this]{
  CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  while(!referenceShutdown){ReferenceDownload task;
   {std::unique_lock lock(referenceDownloadMutex);referenceDownloadWake.wait(lock,[this]{return referenceShutdown||!referenceDownloads.empty();});if(referenceShutdown)break;task=std::move(referenceDownloads.front());referenceDownloads.pop_front();referenceThumbCancel=false;}
   std::string failure;bool cancelled=false;
   try{
    NetworkScope scope(&referenceThumbCancel,std::chrono::steady_clock::now()+std::chrono::seconds(task.preview?30:15),str(task.cfg,"content_mode","standard"));
    auto response=request("GET",joinUrl(str(task.cfg,"api"),"/learning/assets/"+task.asset+(task.preview?"/image":"/thumbnail")),str(task.cfg,"api_key"),"","application/json",&referenceThumbCancel,task.preview?128*1024*1024:2*1024*1024,task.preview?30000:15000);
    if(response.code==429)throw std::runtime_error("서버 요청이 많습니다. 잠시 후 다시 선택하세요.");
    if(referenceThumbCancel)throw std::runtime_error("cancelled");saveImage(task.target,response.data);if(task.preview)registerLocalImage(task.cfg,task.target);
   }catch(const std::exception& x){cancelled=referenceThumbCancel;failure=cancelled?"":userError(x.what());}
   engine->event({"learning_reply",task.project,"reference_download",{{"file",utf8(task.target.wstring())},{"profile",task.profile},{"asset",task.asset},{"preview",task.preview},{"sequence",task.sequence},{"scope",str(task.cfg,"_local_account_scope")},{"mode",str(task.cfg,"content_mode","standard")},{"error",failure},{"cancelled",cancelled}}});
  }
  CoUninitialize();
 });
 referenceDownloadWake.notify_one();
}
void referenceDownloadReply(const Event& event){
 auto file=fs::path(wide(str(event.data,"file")));{std::lock_guard lock(referenceDownloadMutex);auto found=referencePending.find(file);if(found!=referencePending.end()&&found->second==event.data.value("sequence",uint64_t(0)))referencePending.erase(found);}
 if(event.project!=store->active||str(event.data,"scope")!=str(store->settings,"_local_account_scope")||str(event.data,"mode")!=str(store->settings,"content_mode","standard"))return;
 if(event.data.value("preview",false)){
  if(event.data.value("sequence",uint64_t(0))!=referencePreviewSequence)return;
  referencePreviewLoading=false;referencePreviewError=wide(str(event.data,"error"));
  if(!event.data.value("cancelled",false)&&referencePreviewError.empty()&&str(event.data,"profile")==profileId(str(store->settings,"profile_target")))select(file);
  else if(!referencePreviewError.empty()){previewFailed=true;status=referencePreviewError;}
 }else if(!str(event.data,"error").empty())referenceThumbFailures.insert(str(event.data,"profile")+"/"+(str(event.data,"mode")=="hardcore"?"hardcore":"standard")+"/"+str(event.data,"asset"));
 if(hwnd)InvalidateRect(hwnd,nullptr,FALSE);
}
std::set<std::string> selectedReferences,referenceThumbFailures;
std::string referenceSelectionProfile;float referenceScroll=0;Rect referenceListArea;
fs::path referenceCache(const std::string& profile,const std::string& asset,bool thumb=true)const{
 if(profileId(profile).size()!=32||profileId(asset).size()!=32)throw std::runtime_error("이미지 식별자 오류");
 return store->root/"cache/learning"/profile/(str(store->settings,"content_mode")=="hardcore"?"hardcore":"standard")/(asset+(thumb?".thumb.png":".png"));
}
std::wstring trainingState(const json& data)const{
 auto setup=data.value("training_setup",json::object());auto reason=str(setup,"reason");
 if(reason=="owner_not_allowed")return L"서버에서 이 계정의 학습 허용 설정이 필요합니다.";
 if(reason=="setup_required")return L"서버 학습 환경 설정이 필요합니다.";
 if(reason=="pipeline_mismatch")return L"프로필과 서버의 학습 모델이 다릅니다.";
 auto list=data.value("training",json::array());if(!list.empty()){
  auto state=str(list[0],"state");if(state=="running")return L"학습 중";if(state=="queued")return L"학습 대기 중";
  if(state=="completed")return L"학습 완료";if(state=="failed")return L"학습 실패 · 상태 확인";
  if(state=="blocked")return L"서버 학습 설정을 확인하세요.";
  if(state=="cancelled")return L"학습 중지됨";
 }
 int count=data.value("dataset_count",int(data.value("references",json::array()).size()));int minimum=setup.value("minimum_images",5);
 return count<minimum?L"학습 이미지 "+std::to_wstring(count)+L" / "+std::to_wstring(minimum)+L"장":L"학습 준비됨";
}
void loadReferenceProfile(){save();referenceThumbFailures.clear();referenceScroll=0;selectedReferences.clear();featureRequest("profile","GET","/learning/profiles/"+activeProfile());}
void startReferenceTransfer(std::vector<fs::path> files,std::vector<std::string> deletions={}){
 if(learningBusy)return;if(files.size()>100)throw std::runtime_error("이미지는 한 번에 최대 100장까지 추가할 수 있습니다.");
 save();auto profile=activeProfile(),project=store->active;auto cfg=store->settings;auto root=store->root;auto* e=engine;
 for(auto& file:files){std::error_code ec;auto size=fs::file_size(file,ec);if(ec||!size||size>25*1024*1024||!imagePath(file))throw std::runtime_error("이미지는 장당 25MB 이하의 PNG·JPG·WEBP 파일을 선택하세요.");}
 cancelReferenceDownloads();if(referenceTransferThread.joinable())referenceTransferThread.join();referenceTransferCancel=false;referenceUploadRunning=true;learningBusy=true;learningNote=files.empty()?L"이미지 삭제 중…":L"이미지 추가 0 / "+std::to_wstring(files.size());
 for(auto& field:fields)if(field.key=="profile_target")EnableWindow(field.hwnd,FALSE);
 referenceTransferThread=std::thread([this,files=std::move(files),deletions=std::move(deletions),profile,project,cfg,root,e]{
  CoInitializeEx(nullptr,COINIT_MULTITHREADED);json reply=json::object();std::string message;int completed=0;
  try{NetworkScope scope(&referenceTransferCancel,std::chrono::steady_clock::now()+std::chrono::minutes(30),str(cfg,"content_mode","standard"));
   for(auto& file:files){Multipart form;form.file("image",file);form.field("caption",str(cfg,"profile_caption"));form.field("defer_training","true");
    auto response=request("POST",joinUrl(str(cfg,"api"),"/learning/profiles/"+profile+"/references"),str(cfg,"api_key"),form.finish(),form.type(),&referenceTransferCancel,1024*1024,120000);
    if(response.code==429)throw std::runtime_error("요청 또는 이미지 보관 한도에 도달했습니다.");response.object();completed++;
    e->event({"learning_reply",project,"reference_progress",{{"done",completed},{"total",files.size()}}});
   }
   for(size_t begin=0;begin<deletions.size();begin+=100){json ids=json::array();for(size_t i=begin;i<std::min(deletions.size(),begin+100);i++)ids.push_back(deletions[i]);
    auto response=request("POST",joinUrl(str(cfg,"api"),"/learning/profiles/"+profile+"/references/delete"),str(cfg,"api_key"),json{{"asset_ids",ids}}.dump(),"application/json",&referenceTransferCancel,1024*1024,120000);
    if(response.code==429)throw std::runtime_error("잠시 후 다시 삭제하세요.");response.object();completed+=int(ids.size());
    for(auto& id:ids){auto asset=id.get<std::string>();for(auto suffix:{".png",".thumb.png"})for(auto mode:{"standard","hardcore",""}){std::error_code ec;fs::remove(root/"cache/learning"/profile/mode/(asset+suffix),ec);}}
   }
  }catch(const std::exception& x){message=referenceTransferCancel?"중지했습니다.":userError(x.what());}
  try{NetworkScope finish(&referenceShutdown,std::chrono::steady_clock::now()+std::chrono::seconds(30),str(cfg,"content_mode","standard"));
   reply=request("GET",joinUrl(str(cfg,"api"),"/learning/profiles/"+profile),str(cfg,"api_key"),"","application/json",nullptr,16*1024*1024,30000).object();
   if(message.empty()&&!referenceTransferCancel&&!files.empty()&&(flag(reply,"auto_train")||str(reply,"auto_train")=="1")&&reply.value("training_setup",json::object()).value("ready",false)){
    auto result=request("POST",joinUrl(str(cfg,"api"),"/learning/profiles/"+profile+"/train"),str(cfg,"api_key"),"{}","application/json",nullptr,1024*1024,30000);if(result.code!=429)reply["training"]=json::array({result.object()});
   }
  }catch(const std::exception& x){if(message.empty())message=userError(x.what());}
  e->event({"learning_reply",project,"reference_batch",{{"reply",reply},{"message",message},{"done",completed},{"deleting",!deletions.empty()}}});CoUninitialize();
 });
}
void deleteSelectedReferences(){
 if(selectedReferences.empty())throw std::runtime_error("삭제할 이미지를 선택하세요.");
 if(MessageBoxW(hwnd,(std::to_wstring(selectedReferences.size())+L"장의 학습 이미지와 설명 파일을 삭제할까요?\n이미 학습한 결과를 바꾸려면 다시 학습해야 합니다.").c_str(),L"학습 이미지 삭제",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;
 startReferenceTransfer({},std::vector<std::string>(selectedReferences.begin(),selectedReferences.end()));
}
static fs::path fetchReferenceThumbnail(const json& cfg,const fs::path& root,const std::string& profile,const std::string& asset,std::atomic_bool* cancel){
 NetworkScope mode(cancel,std::chrono::steady_clock::now()+std::chrono::seconds(20),str(cfg,"content_mode","standard"));
 auto response=request("GET",joinUrl(str(cfg,"api"),"/learning/assets/"+asset+"/thumbnail"),str(cfg,"api_key"),"","application/json",cancel,2*1024*1024,15000);
 auto target=root/"cache/learning"/profile/(str(cfg,"content_mode")=="hardcore"?"hardcore":"standard")/(asset+".thumb.png");saveImage(target,response.data);return target;
}
void requestReferenceThumbnails(const std::string& profile,const std::vector<json>& rows){
 if(!hwnd||!IsWindowVisible(hwnd)||rows.empty()||learningBusy)return;
 for(auto& row:rows){auto asset=str(row,"id");auto target=referenceCache(profile,asset);queueReferenceDownload({store->settings,store->root,target,store->active,profile,asset,false,0});}
}
bool referencePreviewMode()const{return page==7&&consistencyView==2;}
void showReferencePreview(const std::string& key,const std::string& asset){
 auto cache=referenceCache(key,asset,false);focusPreview=true;referencePreviewError.clear();referencePreviewLoading=false;++referencePreviewSequence;
 if(fs::exists(cache)){registerLocalImage(store->settings,cache);select(cache);return;}
 selected.clear();preview.Reset();previewFailed=false;referencePreviewLoading=true;
 queueReferenceDownload({store->settings,store->root,cache,store->active,key,asset,true,referencePreviewSequence});
}
bool referencePreviewSelected()const{
 auto profile=profileId(str(store->settings,"profile_target"));
 return !selected.empty()&&!profile.empty()&&selected.parent_path()==store->root/"cache/learning"/profile/(str(store->settings,"content_mode")=="hardcore"?"hardcore":"standard")&&fs::exists(selected);
}
void paintReferencePreview(float rx){
 bool available=referencePreviewSelected();
 browser={};galleryDivider={};previewRect={rx+12,60,std::max(40.f,rightWidth-40),std::max(40.f,height-132)};
 renderer.box({rx,8,rightWidth-14,height-16},0x1b1b1c,0x303033,6);label(L"학습 이미지",{rx+14,18,200,28},true);
 renderer.box(previewRect,0x141415,0,6);
 if(available&&preview)renderer.bitmap(preview.Get(),{previewRect.x+10,previewRect.y+10,previewRect.w-20,previewRect.h-20},imageW<=256&&imageH<=256);
 else label(referencePreviewLoading?L"이미지 다운로드 중…":!referencePreviewError.empty()?referencePreviewError:available?L"이미지 표시 중…":L"이미지를 선택하세요",{previewRect.x+15,previewRect.y+previewRect.h/2,previewRect.w-30,30},false,0xdddddd,14);
 button({rx+12,height-56,100,36},L"저장",[this]{exportImage();},false,available);
 button({rx+120,height-56,100,36},L"확대",[this]{openSelected();},false,available);
}
void paintReferenceList(const LearningLayout& layout,float offset){
 auto key=profileId(str(store->settings,"profile_target"));if(key!=referenceSelectionProfile){referenceSelectionProfile=key;selectedReferences.clear();referenceScroll=0;}
 auto found=referenceProfiles.find(key);static const json empty=json::array();const json& refs=found==referenceProfiles.end()||!found->second.contains("references")?empty:found->second["references"];
 Rect outer{layout.clip.x,offset+layout.referencesY,layout.clip.w,layout.referencesHeight};referenceListArea={outer.x,outer.y+32,outer.w,std::max(30.f,outer.h-32)};
 renderer.box(outer,0x151516,0x353538,4);label(L"선택 "+std::to_wstring(selectedReferences.size())+L" / "+std::to_wstring(refs.size())+L"장",{outer.x+10,outer.y,outer.w-96,30},false,0xeeeeee,12);
 button({outer.x+outer.w-82,outer.y+3,76,25},selectedReferences.size()==refs.size()&&!refs.empty()?L"선택 해제":L"전체 선택",[this,key]{auto found=referenceProfiles.find(key);if(found==referenceProfiles.end())return;auto& rows=found->second["references"];if(selectedReferences.size()==rows.size())selectedReferences.clear();else for(auto& row:rows)selectedReferences.insert(str(row,"id"));},false,!learningBusy);
 auto area=referenceListArea;float content=refs.size()*58.f;referenceScroll=std::clamp(referenceScroll,0.f,std::max(0.f,content-area.h));
 renderer.target->PushAxisAlignedClip(area.d2d(),D2D1_ANTIALIAS_MODE_ALIASED);std::vector<json> pending;
 int first=std::max(0,int(referenceScroll/58)),last=std::min(int(refs.size()),first+int(area.h/58)+2);
 for(int i=first;i<last;i++){auto& row=refs[i];auto asset=str(row,"id");Rect r{area.x+4,area.y+i*58-referenceScroll,area.w-12,54};if(selectedReferences.count(asset))renderer.box(r,0x333336,0,3);
  Rect tick{r.x+5,r.y+15,24,24};renderer.box(tick,selectedReferences.count(asset)?0xeeeeee:0x55555a,0,4);if(selectedReferences.count(asset)){renderer.brush->SetColor(color(0x202022));renderer.target->DrawLine(D2D1::Point2F(tick.x+5,tick.y+12),D2D1::Point2F(tick.x+10,tick.y+17),renderer.brush.Get(),2.5f);renderer.target->DrawLine(D2D1::Point2F(tick.x+10,tick.y+17),D2D1::Point2F(tick.x+19,tick.y+7),renderer.brush.Get(),2.5f);}else renderer.box({tick.x+2,tick.y+2,20,20},0x19191a,0,2);
  auto path=referenceCache(key,asset);auto bitmap=thumbs.find(path);Rect thumb{r.x+34,r.y+3,48,48};
  if(bitmap)renderer.bitmap(bitmap->Get(),thumb);else{renderer.box(thumb,0x252528,0,2);if(fs::exists(path)){if(!requested.count(path)&&images.add(path,revision))requested.insert(path);}else if(!referenceThumbFailures.count(key+"/"+(str(store->settings,"content_mode")=="hardcore"?"hardcore":"standard")+"/"+asset)&&pending.size()<12)pending.push_back(row);}
  auto meta=row.value("metadata",json::object());auto name=wide(str(meta,"filename"));if(name.empty())name=L"이미지 "+std::to_wstring(i+1);
  label(name,{r.x+94,r.y+5,r.w-100,22},false,0xeeeeee,12);auto obs=row.value("observations",json::object());label(std::to_wstring(obs.value("width",0))+L" × "+std::to_wstring(obs.value("height",0)),{r.x+94,r.y+28,r.w-100,18},false,0xbbbbbb,11);
  Rect hit{r.x,std::max(r.y,area.y),r.w,std::max(0.f,std::min(r.y+r.h,area.y+area.h)-std::max(r.y,area.y))};Rect checkHit{hit.x,hit.y,30,hit.h};
  buttons.push_back({checkHit,L"",[this,key,asset]{if(!selectedReferences.erase(asset)){selectedReferences.insert(asset);showReferencePreview(key,asset);}},false,!learningBusy});hit.x+=30;hit.w-=30;
  buttons.push_back({hit,L"",[this,key,asset]{showReferencePreview(key,asset);},false,!learningBusy});
 }
 if(refs.empty())label(L"이미지를 추가하세요",{area.x+12,area.y+10,area.w-24,35},false,0xbbbbbb,13);
 renderer.target->PopAxisAlignedClip();requestReferenceThumbnails(key,pending);
 if(content>area.h)renderer.box({area.x+area.w-5,area.y+(area.h-22)*referenceScroll/(content-area.h),3,22},0x77777a,0,1);
}
