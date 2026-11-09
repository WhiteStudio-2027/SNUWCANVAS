#pragma once
#include "app.hpp"
#include <optional>
namespace snuw {
struct StartupCancelled:std::runtime_error {StartupCancelled():std::runtime_error("시작을 취소했습니다.") {}};
// Only the caller dispatches Windows messages. The worker owns local/bootstrap state until joined.
template<class Work> auto startupWork(HWND window,std::atomic_bool& cancel,Work work,std::chrono::seconds budget=std::chrono::seconds(30)){
 using Result=std::invoke_result_t<Work,const std::atomic_bool*>;std::optional<Result> result;std::exception_ptr error;
 struct Signal {HANDLE value=CreateEventW(nullptr,TRUE,FALSE,nullptr);~Signal(){if(value)CloseHandle(value);}}signal;if(!signal.value)throw std::runtime_error("시작 작업 초기화 실패");
 std::thread worker([&]{CoInitializeEx(nullptr,COINIT_MULTITHREADED);try{NetworkScope scope(&cancel,std::chrono::steady_clock::now()+budget);result.emplace(work(&cancel));}catch(...){error=std::current_exception();}SetEvent(signal.value);CoUninitialize();});
 struct Join {std::thread& worker;std::atomic_bool& cancel;~Join(){if(worker.joinable()){cancel=true;worker.join();}}}join{worker,cancel};bool quit=false;int quitCode=0;
 while(WaitForSingleObject(signal.value,0)!=WAIT_OBJECT_0){
  if(!IsWindow(window))cancel=true;DWORD ready=MsgWaitForMultipleObjects(1,&signal.value,FALSE,25,QS_ALLINPUT);if(ready==WAIT_FAILED){cancel=true;Sleep(1);}
  MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){if(message.message==WM_QUIT){cancel=true;quit=true;quitCode=int(message.wParam);continue;}TranslateMessage(&message);DispatchMessageW(&message);}
 }
 worker.join();if(quit)PostQuitMessage(quitCode);if(cancel){if(IsWindow(window))DestroyWindow(window);throw StartupCancelled();}if(error){if(IsWindow(window))DestroyWindow(window);std::rethrow_exception(error);}return std::move(*result);
}
struct StartupAccount {bool signedIn=false,updated=false;json profile=json::object(),usage=json::object();std::wstring warning;};
inline StartupAccount restoreStartupAccount(const std::shared_ptr<Store>& store,const std::atomic_bool* cancel){
 StartupAccount result;auto base=str(store->settings,"api");if(cancel&&*cancel)throw StartupCancelled();accountSession=std::make_shared<AccountSession>(store->root);
 if(!accountSession->available()||base.empty()||(mainServer(base)&&serverAccessFor(base).empty()))return result;
 try{result.profile=request("GET",joinUrl(base,"/api/auth/me"),accountCredential,"","application/json",cancel,128*1024,15000).object();if(str(result.profile,"account_id").empty())throw std::runtime_error("계정 응답 오류");result.signedIn=true;}
 catch(...){if(cancel&&*cancel)throw;result.warning=L"자동 로그인을 확인하지 못했습니다. 서버 연결을 확인하고 다시 로그인하세요.";return result;}
 store->useWorkspace(localAccountScope(base,str(result.profile,"account_id")));store->settings["api_key"]=accountCredential;
 try{result.usage=request("GET",joinUrl(base,"/api/usage"),accountCredential,"","application/json",cancel,64*1024,10000).object();}catch(...){if(cancel&&*cancel)throw;result.warning=L"사용량을 불러오지 못했습니다. 연결 확인 후 갱신하세요.";}
 return result;
}
}
