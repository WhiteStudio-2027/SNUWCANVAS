// App members. Only local build descriptors; never downloads or runs remote code.
std::thread localBuildThread;std::string localBuildRunningHash,localBuildSeen;json localBuildPending=json::object();bool localBuildChecking=false,localBuildRestarting=false;ULONGLONG localBuildPoll=0;
fs::path localBuildRoot=exeDir(),localBuildExecutable=executableFile();
void localBuildReply(const Event& event){localBuildChecking=false;if(event.kind=="local_build_ready"){localBuildPending=event.data;status=L"새 빌드 준비 완료 · 작업이 끝나면 자동 재시작합니다.";}else status=wide(str(event.data,"error","새 빌드를 확인하지 못했습니다."));}
bool localBuildBusy(){return accountWorkspaceBusy()||!trainingJob.empty()||auditionRunning||auditionInFlight||repairActive||repairWorkerActive||seedBusy||seedActive||copyMigrationBusy||promptComposing||interactiveResize||splitter||projectDragging||buttonDown||!IsWindowEnabled(hwnd)||GetCapture()!=nullptr;}
bool localBuildTick(bool now=false){
 if(closing||localBuildRestarting||!hwnd)return false;
 if(!localBuildPending.empty()){
  LASTINPUTINFO input{sizeof input};GetLastInputInfo(&input);if(localBuildBusy()||(!now&&DWORD(GetTickCount()-input.dwTime)<3000))return false;
  save();startLocalBuildHelper(localBuildRoot,localBuildPending);localBuildRestarting=true;PostMessageW(hwnd,WM_CLOSE,0,0);return true;
 }
 if(localBuildChecking||GetTickCount64()<localBuildPoll)return false;localBuildPoll=GetTickCount64()+2000;
 auto target=localBuildExecutable;auto file=localBuildReadyFile(localBuildRoot,target);if(!fs::exists(file))return false;try{auto build=readJson(file);auto hash=str(build,"sha256");if(hash==localBuildRunningHash||hash==localBuildSeen)return false;if(str(build,"target")!=utf8(target.filename().wstring()))throw std::runtime_error("다른 앱용 빌드입니다.");auto statusFile=localBuildStatusFile(localBuildRoot,build);if(fs::exists(statusFile)){auto old=readJson(statusFile);if(str(old,"sha256")==hash&&str(old,"state")=="failed"){localBuildSeen=hash;return false;}}
  if(localBuildThread.joinable())localBuildThread.join();localBuildSeen=hash;localBuildChecking=true;auto root=localBuildRoot;auto* receiver=engine;localBuildThread=std::thread([root,build,receiver]{try{validateLocalBuild(root,build);receiver->event({"local_build_ready","","",build});}catch(const std::exception& error){receiver->event({"local_build_error","","",{{"error",error.what()}}});}});
 }catch(const std::exception& error){status=wide(userError(error.what()));}return false;
}
void applyLocalBuildNow(){localBuildPoll=0;if(localBuildPending.empty()){localBuildSeen.clear();localBuildTick();status=localBuildChecking?L"새 빌드를 확인합니다.":L"적용할 새 빌드가 없습니다.";}else if(!localBuildTick(true))status=L"진행 중인 작업이 끝나면 새 빌드를 적용합니다.";}
