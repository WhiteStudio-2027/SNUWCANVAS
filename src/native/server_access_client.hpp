// Included inside namespace snuw, after the HTTP request implementation.
inline std::vector<BYTE> accessUnhex(const std::string& value){
 if(value.size()%2||value.size()>256)throw std::runtime_error("접속 암호 확인 응답 오류");std::vector<BYTE> out;
 for(size_t i=0;i<value.size();i+=2){auto digit=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("접속 암호 확인 응답 오류");};out.push_back(BYTE(digit(value[i])*16+digit(value[i+1])));}return out;
}
inline std::string serverAccessProof(const std::string& password,const json& challenge){
 auto salt=accessUnhex(str(challenge,"salt"));auto identity=str(challenge,"id"),nonce=str(challenge,"nonce");
 if(salt.size()!=16||identity.size()!=32||nonce.size()!=64||challenge.value("iterations",0)!=600000)throw std::runtime_error("접속 암호 확인 응답 오류");accessUnhex(identity);accessUnhex(nonce);
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;BYTE key[32]{},digest[32]{};std::string result;std::vector<BYTE> object;
 try{
  if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0||BCryptDeriveKeyPBKDF2(algorithm,(PUCHAR)password.data(),ULONG(password.size()),salt.data(),ULONG(salt.size()),600000,key,32,0)<0)throw std::runtime_error("접속 암호 계산 실패");
  DWORD size=0,received=0;if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,(PUCHAR)&size,sizeof size,&received,0)<0)throw std::runtime_error("접속 암호 계산 실패");object.resize(size);
  auto message="snuw-main-access-v1:"+identity+":"+nonce;
  if(BCryptCreateHash(algorithm,&hash,object.data(),size,key,32,0)<0||BCryptHashData(hash,(PUCHAR)message.data(),ULONG(message.size()),0)<0||BCryptFinishHash(hash,digest,32,0)<0)throw std::runtime_error("접속 암호 계산 실패");
  const char* hex="0123456789abcdef";for(auto byte:digest){result+=hex[byte>>4];result+=hex[byte&15];}
  BCryptDestroyHash(hash);hash=nullptr;
 }catch(...){if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);SecureZeroMemory(key,sizeof key);throw;}
 BCryptCloseAlgorithmProvider(algorithm,0);SecureZeroMemory(key,sizeof key);return result;
}
inline void establishServerAccess(const std::string& base,const std::string& password,const std::atomic_bool* cancel){
 auto challenge=request("GET",joinUrl(base,"/api/access/challenge"),"","","application/json",cancel,8192,15000).object();
 if(!challenge.value("required",false))throw std::runtime_error("메인 서버의 접속 암호가 설정되지 않았습니다. 서버 관리자에게 문의하세요.");
 auto proof=serverAccessProof(password,challenge);
 auto response=request("POST",joinUrl(base,"/api/access/unlock"),"",json{{"id",str(challenge,"id")},{"proof",proof}}.dump(),"application/json",cancel,8192,15000).object();auto token=str(response,"token");
 if(token.size()<32||token.size()>128||token.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos)throw std::runtime_error("서버 접속 응답 오류");
 std::lock_guard lock(serverAccessMutex);serverAccessOrigin=base;serverAccessToken=token;serverAccessExpires=GetTickCount64()+ULONGLONG(std::clamp(response.value("expires_in",0),0,43200))*1000;
}
inline int serverAccessTest(const fs::path& folder,const std::string& base){try{
 establishServerAccess(base,"test-shared-password-1234",nullptr);auto result=request("GET",joinUrl(base,"/protected")).object();if(!result.value("ok",false))throw std::runtime_error("Server entry gate did not authorize the request");
 if(!serverAccessFor("http://example.invalid:8000").empty())throw std::runtime_error("Server entry token leaked across origins");
 atomicWrite(folder/"server-access-result.json",json{{"ok",true},{"password_not_transmitted",true},{"cross_origin_blocked",true}}.dump());return 0;
 }catch(const std::exception& e){atomicWrite(folder/"server-access-result.json",json{{"ok",false},{"error",e.what()}}.dump());return 1;}}
