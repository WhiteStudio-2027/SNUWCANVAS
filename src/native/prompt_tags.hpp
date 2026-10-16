#pragma once
#include "common.hpp"
#include <regex>
#include <iomanip>
#include <sstream>
namespace snuw {
struct TagEdit {size_t begin=0,end=0;std::wstring text;bool valid=false;};
inline TagEdit weightedTag(const std::wstring& source,size_t caret,int direction){
 caret=std::min(caret,source.size());size_t begin=0,end=source.size();int round=0,square=0;bool escaped=false;
 for(size_t i=0;i<source.size();i++){wchar_t c=source[i];if(escaped){escaped=false;continue;}if(c==L'\\'){escaped=true;continue;}if(c==L'(')round++;if(c==L')')round=std::max(0,round-1);if(c==L'[')square++;if(c==L']')square=std::max(0,square-1);if((c==L','&&round==0&&square==0)||c==L'\r'||c==L'\n'){if(i<caret)begin=i+1;else{end=i;break;}}}
 while(begin<end&&iswspace(source[begin]))begin++;while(end>begin&&iswspace(source[end-1]))end--;if(begin==end)return {};
 auto tag=source.substr(begin,end-begin);if(tag.front()==L'<'||tag.front()==L'[')return {};double weight=1.;std::wsmatch match;
 if(std::regex_match(tag,match,std::wregex(LR"(^\((.*):\s*([0-9]+(?:\.[0-9]+)?)\s*\)$)"))){tag=match[1];try{weight=std::stod(match[2]);}catch(...){return {};}}
 else if(tag.front()==L'('&&tag.back()==L')'){tag=tag.substr(1,tag.size()-2);weight=1.1;}
 std::wstring replacement=tag;if(direction){weight=std::clamp(std::round((weight+direction*.1)*10)/10.,.1,3.);std::wostringstream out;out<<L'('<<tag<<L':'<<std::fixed<<std::setprecision(1)<<weight<<L')';replacement=out.str();}
 return {begin,end,replacement,true};
}
}
