#pragma once
#include "common.hpp"
namespace snuw {
struct EntityPrefix {bool valid=false;size_t begin=0;std::wstring query;};
inline EntityPrefix entityPrefix(const std::wstring& prefix){
 size_t opened=std::wstring::npos;bool escaped=false;
 for(size_t i=0;i<prefix.size();i++){
  auto c=prefix[i];if(c==L'\r'||c==L'\n'||c==L'/'){opened=std::wstring::npos;escaped=false;continue;}
  if(c==L':'){
   if(i&&prefix[i-1]==L'\\'){escaped=true;opened=std::wstring::npos;continue;}
   if(escaped){escaped=false;continue;}
   if(opened!=std::wstring::npos){opened=std::wstring::npos;continue;}
   if(i&&(iswdigit(prefix[i-1])||prefix[i-1]==L'/'))continue;
   opened=i;
  }else if(opened!=std::wstring::npos&&(i-opened>100||(i==opened+1&&iswspace(c))))opened=std::wstring::npos;
 }
 if(opened==std::wstring::npos)return {};
 auto word=prefix.find_last_of(L" \t\r\n",opened);word=word==std::wstring::npos?0:word+1;auto context=prefix.substr(word,opened-word);
 std::transform(context.begin(),context.end(),context.begin(),::towlower);
 if(context.find(L"://")!=std::wstring::npos||context==L"http"||context==L"https"||context==L"ftp"||context==L"ws"||context==L"wss"||context==L"mailto"||context.rfind(L"mailto:",0)==0||context==L"data"||context.rfind(L"data:",0)==0||context==L"urn"||context.rfind(L"urn:",0)==0)return {};
 return {true,opened,prefix.substr(opened+1)};
}
inline std::wstring entitySearchKey(const std::wstring& text){
 std::wstring folded=text;int size=FoldStringW(MAP_FOLDCZONE,text.data(),int(text.size()),nullptr,0);
 if(size>0){folded.resize(size);if(!FoldStringW(MAP_FOLDCZONE,text.data(),int(text.size()),folded.data(),size))folded=text;}
 size=LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_LOWERCASE,folded.data(),int(folded.size()),nullptr,0,nullptr,nullptr,0);
 if(size>0){std::wstring lower(size,L'\0');if(LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_LOWERCASE,folded.data(),int(folded.size()),lower.data(),size,nullptr,nullptr,0))return lower;}
 return folded;
}
inline std::vector<std::wstring> matchingEntities(const json& rows,const std::wstring& prefix){
 std::vector<std::wstring> result;auto key=entitySearchKey(prefix);
 if(!rows.is_array())return result;
 std::vector<std::wstring> names;std::map<std::wstring,int> counts;
 for(auto& row:rows){auto name=wide(str(row,"name"));auto id=str(row,"id");if(id.size()!=32||id.find_first_not_of("0123456789abcdef")!=std::string::npos)continue;auto start=name.find_first_not_of(L" \t\r\n"),end=name.find_last_not_of(L" \t\r\n");if(start==std::wstring::npos)continue;name=name.substr(start,end-start+1);
  if(name.size()>100||name.find_first_of(L":/\\\r\n")!=std::wstring::npos||std::any_of(name.begin(),name.end(),[](wchar_t c){return c<32;}))continue;auto folded=entitySearchKey(name);bool numeric=std::all_of(folded.begin(),folded.end(),[](wchar_t c){WORD type=0;return GetStringTypeW(CT_CTYPE1,&c,1,&type)&&(type&C1_DIGIT);});if(numeric)continue;names.push_back(name);counts[folded]++;
 }
 for(auto& name:names){auto folded=entitySearchKey(name);if(counts[folded]==1&&folded.rfind(key,0)==0)result.push_back(name);if(result.size()>=20)break;
 }return result;
}
}
