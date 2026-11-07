#pragma once
#include "common.hpp"
namespace snuw {
inline bool textEntityKind(const std::string& kind){return kind=="object"||kind=="place";}
inline std::string entityKind(const std::string& value){auto split=value.find(" | ");return split==std::string::npos?value:value.substr(split+3);}
inline std::vector<std::wstring> entityKindChoices(){return {L"캐릭터 | character",L"사물 | object",L"장소 | place",L"화풍 | style",L"제품 (참조·학습) | product",L"브랜드 (참조·학습) | brand"};}
inline std::string entityKindLabel(const std::string& value){auto kind=entityKind(value);for(auto& choice:entityKindChoices())if(entityKind(utf8(choice))==kind)return utf8(choice);return value;}
struct EntityFieldSpec {const char* key;const char* setting;const wchar_t* label;};
inline const std::vector<EntityFieldSpec> entityFields={{"appearance","profile_appearance",L"외형"},{"clothing","profile_clothing",L"의상"},{"expression","profile_expression",L"표정"},{"pose","profile_pose",L"자세"},{"location","profile_location",L"장소"},{"lighting","profile_lighting",L"조명"},{"style","profile_style",L"화풍"}};
}
