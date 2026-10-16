#pragma once
#include "model.hpp"
namespace snuw {
struct Internet {HINTERNET h=nullptr;Internet()=default;explicit Internet(HINTERNET p):h(p){}~Internet(){if(h)WinHttpCloseHandle(h);}Internet(const Internet&)=delete;operator HINTERNET()const{return h;}};
struct Url {std::wstring host,path;INTERNET_PORT port;bool secure;explicit Url(const std::string& value){auto s=wide(value);URL_COMPONENTS u{};u.dwStructSize=sizeof u;u.dwHostNameLength=u.dwUrlPathLength=u.dwExtraInfoLength=DWORD(-1);if(!WinHttpCrackUrl(s.c_str(),0,0,&u)||(u.nScheme!=INTERNET_SCHEME_HTTP&&u.nScheme!=INTERNET_SCHEME_HTTPS)||u.dwUserNameLength||u.dwPasswordLength)throw std::runtime_error("HTTP(S) 서버 주소를 확인하세요.");host.assign(u.lpszHostName,u.dwHostNameLength);path.assign(u.lpszUrlPath,u.dwUrlPathLength);if(u.dwExtraInfoLength)path.append(u.lpszExtraInfo,u.dwExtraInfoLength);if(path.empty())path=L"/";port=u.nPort;secure=u.nScheme==INTERNET_SCHEME_HTTPS;}};
struct Response {DWORD code=0;std::string data;std::string retry;json object()const{if(data.empty())return json::object();return json::parse(data);} };
inline std::string joinUrl(const std::string& base,const std::string& path){if(path.rfind("http://",0)==0||path.rfind("https://",0)==0)return path;auto s=base;while(!s.empty()&&s.back()=='/')s.pop_back();return s+(path.empty()||path[0]=='/'?"":"/")+path;}
inline bool sameOrigin(const std::string& a,const std::string& b){Url x(a),y(b);return _wcsicmp(x.host.c_str(),y.host.c_str())==0&&x.port==y.port&&x.secure==y.secure;}

inline thread_local const std::atomic_bool* networkStop=nullptr;
inline thread_local auto networkDeadline=std::chrono::steady_clock::time_point::max();
struct NetworkScope {
 const std::atomic_bool* oldStop;std::chrono::steady_clock::time_point oldDeadline;
 NetworkScope(const std::atomic_bool* flag,std::chrono::steady_clock::time_point deadline):oldStop(networkStop),oldDeadline(networkDeadline){networkStop=flag;networkDeadline=deadline;}
 ~NetworkScope(){networkStop=oldStop;networkDeadline=oldDeadline;}
};
struct AsyncTransfer {
 HINTERNET handle=nullptr;std::mutex mutex;std::condition_variable cv;bool ready=false,closed=false,armed=false;DWORD error=0,bytes=0;char buffer[65536];
 explicit AsyncTransfer(HINTERNET h):handle(h){if(!h)throw std::runtime_error("서버 요청 준비 실패");}
 static void CALLBACK callback(HINTERNET,DWORD_PTR context,DWORD status,LPVOID data,DWORD length){
  if(!context)return;auto* self=reinterpret_cast<AsyncTransfer*>(context);std::lock_guard lock(self->mutex);
  if(status==WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)self->closed=true;
  else if(status==WINHTTP_CALLBACK_STATUS_REQUEST_ERROR){self->error=((WINHTTP_ASYNC_RESULT*)data)->dwError;self->ready=true;}
  else if(status==WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE||status==WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE||status==WINHTTP_CALLBACK_STATUS_READ_COMPLETE){self->bytes=status==WINHTTP_CALLBACK_STATUS_READ_COMPLETE?length:0;self->ready=true;}
  self->cv.notify_all();
 }
 void start(){DWORD_PTR context=(DWORD_PTR)this;check(WinHttpSetOption(handle,WINHTTP_OPTION_CONTEXT_VALUE,&context,sizeof context)?S_OK:E_FAIL,"통신 컨텍스트 초기화 실패");if(WinHttpSetStatusCallback(handle,callback,WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS|WINHTTP_CALLBACK_FLAG_HANDLES,0)==WINHTTP_INVALID_STATUS_CALLBACK)throw std::runtime_error("비동기 통신 초기화 실패");armed=true;}
 template<class F> DWORD operation(F invoke,const std::atomic_bool* stop,std::chrono::steady_clock::time_point deadline){
  if(stop&&*stop)throw std::runtime_error("취소됨");if(std::chrono::steady_clock::now()>=deadline)throw std::runtime_error("작업 제한 시간 초과");
  {std::lock_guard lock(mutex);ready=false;error=bytes=0;}
  if(!invoke()&&GetLastError()!=ERROR_IO_PENDING)throw std::runtime_error("서버 통신 실패 ("+std::to_string(GetLastError())+")");
  std::unique_lock lock(mutex);for(;;){if(stop&&*stop)throw std::runtime_error("취소됨");if(std::chrono::steady_clock::now()>=deadline)throw std::runtime_error("작업 제한 시간 초과");if(ready)break;cv.wait_for(lock,std::chrono::milliseconds(25));}
  if(error)throw std::runtime_error("서버 통신 실패 ("+std::to_string(error)+")");return bytes;
 }
 ~AsyncTransfer(){if(handle){WinHttpCloseHandle(handle);if(armed){std::unique_lock lock(mutex);cv.wait(lock,[&]{return closed;});}}}
 operator HINTERNET()const{return handle;}
};
inline Response request(const std::string& method,const std::string& url,const std::string& key="",const std::string& body="",const std::string& content="application/json",const std::atomic_bool* stop=nullptr,size_t limit=256*1024*1024,int timeoutMs=30000,std::function<void(const std::string&)> onChunk={}){
 if(!stop)stop=networkStop;auto deadline=std::min(networkDeadline,std::chrono::steady_clock::now()+std::chrono::milliseconds(timeoutMs));
 Url u(url);if(key.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("비밀키에 줄바꿈이 있습니다.");
 Internet session(WinHttpOpen(L"SNUWCANVAS/Native",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,WINHTTP_FLAG_ASYNC));if(!session.h)throw std::runtime_error("네트워크 초기화 실패");WinHttpSetTimeouts(session,10000,10000,30000,30000);
 Internet connection(WinHttpConnect(session,u.host.c_str(),u.port,0));AsyncTransfer req(WinHttpOpenRequest(connection,wide(method).c_str(),u.path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,u.secure?WINHTTP_FLAG_SECURE:0));req.start();DWORD redirect=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(req,WINHTTP_OPTION_REDIRECT_POLICY,&redirect,sizeof redirect);
 std::wstring headers=L"Content-Type: "+wide(content)+L"\r\n";if(!key.empty())headers+=L"X-API-Key: "+wide(key)+L"\r\n";
 req.operation([&]{return WinHttpSendRequest(req,headers.c_str(),DWORD(-1),body.empty()?WINHTTP_NO_REQUEST_DATA:(void*)body.data(),DWORD(body.size()),DWORD(body.size()),(DWORD_PTR)&req);},stop,deadline);
 req.operation([&]{return WinHttpReceiveResponse(req,nullptr);},stop,deadline);
 Response out;DWORD n=sizeof out.code;WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&out.code,&n,WINHTTP_NO_HEADER_INDEX);
 wchar_t retry[32];n=sizeof retry;if(WinHttpQueryHeaders(req,WINHTTP_QUERY_RETRY_AFTER,nullptr,retry,&n,nullptr))out.retry=utf8(retry);
 for(;;){auto got=req.operation([&]{return WinHttpReadData(req,req.buffer,sizeof req.buffer,nullptr);},stop,deadline);if(!got)break;if(out.data.size()+got>limit)throw std::runtime_error("서버 응답 크기 제한 초과");if(onChunk&&out.code<300)onChunk(std::string(req.buffer,got));else out.data.append(req.buffer,got);}
 if(out.code>=300){std::string reason;try{auto j=out.object();reason=str(j,"error",str(j,"detail"));}catch(...){}if(out.code==429)return out;throw std::runtime_error("HTTP "+std::to_string(out.code)+(reason.empty()?"":" · "+reason));}return out;
}

struct Multipart {std::string boundary="snuw"+id(),data;void field(const std::string& name,const std::string& value){data+="--"+boundary+"\r\nContent-Disposition: form-data; name=\""+name+"\"\r\n\r\n"+value+"\r\n";}void file(const std::string& name,const fs::path& p,size_t max=25*1024*1024){auto bytes=readFile(p);if(bytes.empty()||bytes.size()>max)throw std::runtime_error("이미지 파일 크기를 확인하세요.");auto filename=utf8(p.filename().wstring());for(auto& c:filename)if(c=='\r'||c=='\n'||c=='"')c='_';data+="--"+boundary+"\r\nContent-Disposition: form-data; name=\""+name+"\"; filename=\""+filename+"\"\r\nContent-Type: "+mime(p)+"\r\n\r\n"+bytes+"\r\n";}std::string type()const{return "multipart/form-data; boundary="+boundary;}std::string finish(){return data+"--"+boundary+"--\r\n";}};
inline Multipart generationForm(const json& cfg,const json& state,const std::string& requestId){validate(cfg);Multipart m;for(auto key:{"seed","width","height","ip_weight","ip_end","pose_strength","pose_end","count"})m.field(key,str(cfg,key));for(auto& [key,label]:sections)m.field(key,str(state["drafts"],key));m.field("negative",str(state,"negative"));for(auto key:{"enhance","use_default_negative","quality_tags"})m.field(key,flag(cfg,key)?"true":"false");m.field("request_id",requestId);m.field("advanced_settings",controls(cfg).dump());auto refs=state["project"].value("references",json::array());if(refs.size()>10)throw std::runtime_error("참조 이미지는 최대 10장입니다.");json options=json::array();int pose=0,other=0;for(auto& ref:refs){auto role=str(ref,"role");if(role=="pose")pose++;else other++;if(role!="pose"&&role!="character"&&role!="clothing"&&role!="style")throw std::runtime_error("참조 용도 오류");double weight=std::stod(str(ref,"weight")),end=std::stod(str(ref,"end"));if(!std::isfinite(weight)||weight<0||weight>2||!std::isfinite(end)||end<=0||end>1)throw std::runtime_error("참조 강도 범위 오류");m.file("references",wide(str(ref,"path")));options.push_back({{"role",role},{"weight",weight},{"end",end}});}if(str(cfg,"workflow_profile").find("FLUX")==std::string::npos&&(pose>1||other>3))throw std::runtime_error("참조는 캐릭터·의상·화풍 3장, 자세 1장까지 가능합니다.");m.field("reference_options",options.dump());if(!str(cfg,"initial_image").empty())m.file("initial_image",wide(str(cfg,"initial_image")));return m;}
inline void saveImage(const fs::path& target,const std::string& bytes){Com<IWICImagingFactory> wic;check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&wic)),"이미지 디코더 초기화 실패");Com<IWICStream> source;wic->CreateStream(&source);check(source->InitializeFromMemory((BYTE*)bytes.data(),DWORD(bytes.size())),"이미지 응답 오류");Com<IWICBitmapDecoder> decoder;check(wic->CreateDecoderFromStream(source.Get(),nullptr,WICDecodeMetadataCacheOnDemand,&decoder),"이미지가 아닌 서버 응답입니다.");Com<IWICBitmapFrameDecode> image;decoder->GetFrame(0,&image);UINT w,h;image->GetSize(&w,&h);if(!w||!h||uint64_t(w)*h>100000000)throw std::runtime_error("이미지 크기를 확인하세요.");fs::create_directories(target.parent_path());auto tmp=target;tmp+=L"."+wide(id())+L".tmp";try{Com<IWICStream> stream;wic->CreateStream(&stream);check(stream->InitializeFromFilename(tmp.c_str(),GENERIC_WRITE),"이미지 저장 실패");Com<IWICBitmapEncoder> encoder;wic->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder);check(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache),"PNG 인코더 오류");Com<IWICBitmapFrameEncode> frame;Com<IPropertyBag2> props;encoder->CreateNewFrame(&frame,&props);frame->Initialize(props.Get());frame->SetSize(w,h);WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;frame->SetPixelFormat(&format);check(frame->WriteSource(image.Get(),nullptr),"PNG 저장 오류");check(frame->Commit(),"이미지 저장 오류");check(encoder->Commit(),"이미지 저장 오류");frame.Reset();encoder.Reset();stream.Reset();if(!MoveFileExW(tmp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("이미지 파일 교체 오류");}catch(...){fs::remove(tmp);throw;}}
struct Event {std::string kind,project,text;json data;};
struct Task {std::string project,requestId;json cfg,state;bool editing=false;};
class Engine {
public:
 std::mutex mutex;std::condition_variable cv;std::deque<Task> tasks;std::deque<Event> events;std::atomic_bool stopping=false,cancel=false,busy=false,cancelling=false;std::string activeProject,activeRequest,jobId,jobApi,jobKey;std::thread worker,cancelWorker;
 Engine(){worker=std::thread([this]{CoInitializeEx(nullptr,COINIT_MULTITHREADED);loop();CoUninitialize();});}
 ~Engine(){stopping=true;cancel=true;cv.notify_all();if(worker.joinable())worker.join();if(cancelWorker.joinable())cancelWorker.join();}
 void event(Event e){std::lock_guard lock(mutex);events.push_back(std::move(e));}
 std::deque<Event> take(){std::lock_guard lock(mutex);std::deque<Event> out;out.swap(events);return out;}
 void enqueue(Task t){std::lock_guard lock(mutex);if(cancelling)throw std::runtime_error("서버 중지 확인 중입니다. 잠시 후 다시 시도하세요.");tasks.push_back(std::move(t));cv.notify_one();}
 bool locked(const std::string& p){std::lock_guard lock(mutex);if(busy&&p==activeProject)return true;for(auto& t:tasks)if(t.project==p)return true;return false;}
 void stop(){
  std::string api,key,token,jid,project;{std::lock_guard lock(mutex);tasks.clear();cancel=true;cv.notify_all();if(!busy||cancelling.exchange(true))return;api=jobApi;key=jobKey;token=activeRequest;jid=jobId;project=activeProject;}
  event({"cancelling",project,"로컬 작업 중지 · 서버 취소 전송 중",{}});
  if(cancelWorker.joinable())cancelWorker.join();cancelWorker=std::thread([this,api,key,token,jid,project]{bool confirmed=false;std::string reason;for(int attempt=0;attempt<3&&!confirmed;attempt++){try{try{if(request("POST",joinUrl(api,"/client-requests/"+token+"/cancel"),key,"{}","application/json",nullptr,1024*1024,5000).code==429)throw std::runtime_error("서버 취소 요청 제한 (HTTP 429)");}catch(const std::exception& e){if(std::string(e.what()).find("HTTP 404")==std::string::npos||jid.empty())throw;if(request("POST",joinUrl(api,"/jobs/"+jid+"/cancel"),key,"{}","application/json",nullptr,1024*1024,5000).code==429)throw std::runtime_error("서버 취소 요청 제한 (HTTP 429)");}confirmed=true;}catch(const std::exception& e){reason=e.what();if(attempt<2)std::this_thread::sleep_for(std::chrono::milliseconds(250));}}
   event({"cancelled",project,confirmed?"중지됨 · 서버 취소 확인":"로컬 중지됨 · 서버 취소 확인 실패: "+reason,{{"confirmed",confirmed}}});cancelling=false;
  });
 }
 bool wait(int ms){for(int i=0;i<ms/50&&!stopping&&!cancel&&std::chrono::steady_clock::now()<networkDeadline;i++)std::this_thread::sleep_for(std::chrono::milliseconds(50));return !stopping&&!cancel;}
 fs::path download(const Task& t,const json& result,const std::string& prefix){auto base=str(t.cfg,"api"),url=joinUrl(base,str(result,"image_url"));auto response=request("GET",url,sameOrigin(base,url)?str(t.cfg,"api_key"):"","","application/json",&cancel);auto path=fs::path(wide(str(t.cfg,"output")))/(prefix+timestamp()+"_"+id().substr(0,8)+".png");saveImage(path,response.data);if(flag(t.cfg,"save_metadata")){auto meta=path;meta.replace_extension(".json");json data={{"settings",t.cfg},{"project",t.state["project"]},{"drafts",t.state["drafts"]},{"negative",t.state["negative"]},{"result",result}};data["settings"].erase("api_key");atomicWrite(meta,data.dump(2));}event({"image",t.project,utf8(path.wstring()),result});return path;}
 void run(Task t){if(cancel||stopping)return;int seconds=7200;try{seconds=std::clamp(std::stoi(str(t.cfg,"job_timeout_seconds","7200")),1,604800);}catch(...){}NetworkScope scope(&cancel,std::chrono::steady_clock::now()+std::chrono::seconds(seconds));auto base=str(t.cfg,"api"),key=str(t.cfg,"api_key");std::string session=str(t.cfg,"editing_session"),pending=str(t.cfg,"editing_job");json initial;
  if(t.editing){if(session.empty()){Multipart upload;upload.file("image",wide(str(t.cfg,"editing_image")),20*1024*1024);session=str(request("POST",joinUrl(base,"/sessions"),key,upload.finish(),upload.type(),&cancel).object(),"session_id");event({"settings",t.project,"",{{"editing_session",session}}});}if(!pending.empty())initial={{"id",pending}};else{json seed=nullptr;if(str(t.cfg,"seed")!="-1")seed=std::stoll(str(t.cfg,"seed"));initial=request("POST",joinUrl(base,"/sessions/"+session+"/edit?background=true"),key,json{{"text",str(t.cfg,"editing_instruction")},{"request_id",t.requestId},{"seed",seed}}.dump(),"application/json",nullptr).object();}}
  else {auto form=generationForm(t.cfg,t.state,t.requestId);initial=request("POST",joinUrl(base,"/jobs"),key,form.finish(),form.type(),nullptr).object();}
  auto jid=str(initial,"id");if(jid.empty())throw std::runtime_error("서버가 작업 ID를 반환하지 않았습니다.");{std::lock_guard lock(mutex);jobId=jid;jobApi=base;jobKey=key;}if(t.editing)event({"settings",t.project,"",{{"editing_session",session},{"editing_job",jid},{"editing_request",t.requestId}}});
  size_t received=0;bool terminal=false;std::string pendingLine;
  auto consume=[&](const std::string& bytes){pendingLine+=bytes;if(pendingLine.size()>16*1024*1024)throw std::runtime_error("상태 응답 크기 제한 초과");size_t end;
   while((end=pendingLine.find('\n'))!=std::string::npos){auto line=pendingLine.substr(0,end);pendingLine.erase(0,end+1);if(line.empty()||line=="\r")continue;
    auto j=json::parse(line);auto status=str(j,"state");event({"progress",t.project,status,j});auto results=j.value("results",json::array());
    if(!t.editing)for(size_t i=received;i<results.size();++i){download(t,results[i],str(t.cfg,"version","v9")+"_");received++;}
    if(status=="completed"){if(t.editing&&!results.empty()){download(t,results.back(),"edit_");event({"history_ready",t.project,session,request("GET",joinUrl(base,"/sessions/"+session+"/history"),key).object()});event({"settings",t.project,"",{{"editing_job",""},{"editing_request",""}}});}terminal=true;}
    else if(status=="failed"||status=="cancelled"){if(t.editing)event({"settings",t.project,"",{{"editing_job",""},{"editing_request",""}}});throw std::runtime_error(str(j,"error","서버 작업 중단"));}
    else if(status!="queued"&&status!="running")throw std::runtime_error("알 수 없는 서버 작업 상태: "+status);
   }
  };
  auto stream=request("GET",joinUrl(base,"/jobs/"+jid+"/events"),key,"","application/json",&cancel,256*1024*1024,seconds*1000,consume);
  if(stream.code==429)throw std::runtime_error("서버 요청 제한 · 잠시 후 다시 생성해 주세요.");
  if(!terminal)throw std::runtime_error("서버 상태 연결이 완료 전에 끊겼습니다. 진행 상태를 반복 조회하지 않습니다.");

 }
 void loop(){while(!stopping){Task task;{std::unique_lock lock(mutex);cv.wait(lock,[&]{return stopping||!tasks.empty();});if(stopping)return;task=std::move(tasks.front());tasks.pop_front();activeProject=task.project;activeRequest=task.requestId;jobApi=str(task.cfg,"api");jobKey=str(task.cfg,"api_key");jobId.clear();cancel=false;busy=true;}event({"started",task.project,"",{}});try{run(task);event({"done",task.project,cancel?"중지됨":"완료",{}});}catch(const std::exception& e){if(!cancel){event({"error",task.project,e.what(),{}});stop();}}busy=false;}}
};
}
