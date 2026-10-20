#pragma once
#include "identity.hpp"
#include <ctime>
namespace snuw {
// Tokens stay outside Store/settings and are bound to one server origin.
inline constexpr const char* accountCredential="@account-session";
class AccountSession {
 fs::path path;json state=json::object();std::mutex mutex;
 void persist(){auto raw=state.dump();DATA_BLOB input{DWORD(raw.size()),(BYTE*)raw.data()},output{};if(!CryptProtectData(&input,L"snuwcanvas account session",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))throw std::runtime_error("로그인 정보 보호 실패");try{atomicWrite(path,std::string((char*)output.pbData,output.cbData));}catch(...){LocalFree(output.pbData);throw;}LocalFree(output.pbData);}
 void assign(const std::string& server,const json& response){auto access=str(response,"access_token"),refresh=str(response,"refresh_token");int seconds=response.value("expires_in",0);if(access.size()<20||access.size()>4096||refresh.size()<40||refresh.size()>200||access.find_first_of("\r\n")!=std::string::npos||seconds<1||seconds>3600)throw std::runtime_error("로그인 응답 형식 오류");state={{"server",server},{"access_token",access},{"refresh_token",refresh},{"expires_at",std::time(nullptr)+seconds},{"account_id",str(response,"account_id")}};persist();}
public:
 explicit AccountSession(const fs::path& root):path(root/"config/account_session.bin"){
  if(!fs::exists(path))return;auto raw=readFile(path);if(raw.empty()||raw.size()>65536)throw std::runtime_error("로그인 저장 파일 오류. 다시 로그인하세요.");DATA_BLOB input{DWORD(raw.size()),(BYTE*)raw.data()},output{};if(!CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output))throw std::runtime_error("현재 Windows 사용자로 로그인 정보를 열 수 없습니다.");try{state=json::parse(std::string((char*)output.pbData,output.cbData));}catch(...){LocalFree(output.pbData);throw;}LocalFree(output.pbData);
 }
 void accept(const std::string& server,const json& response){std::lock_guard lock(mutex);assign(server,response);}
 bool available(){std::lock_guard lock(mutex);return !str(state,"refresh_token").empty();}
 void clear(){std::lock_guard lock(mutex);state=json::object();persist();}
 std::string authorization(const std::string& url,std::function<bool(const std::string&,const std::string&)> origin,std::function<json(const std::string&,const std::string&)> refresh){std::lock_guard lock(mutex);auto server=str(state,"server");if(server.empty()||!origin(server,url))throw std::runtime_error("이 서버에 로그인하세요. 다른 서버로 로그인 정보를 보내지 않습니다.");if(state.value("expires_at",int64_t(0))<=std::time(nullptr)+30){auto token=str(state,"refresh_token");if(token.empty())throw std::runtime_error("로그인이 만료됐습니다. 다시 로그인하세요.");auto response=refresh(server,token);assign(server,response);}return str(state,"access_token");}
};
inline std::shared_ptr<AccountSession> accountSession;
}
