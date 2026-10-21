#pragma once
#include "common.hpp"
#include "foundation_generated.hpp"
namespace snuw {
inline const json& modelGuards(){static const json value=json::parse(MODEL_GUARDS_JSON);return value;}
inline std::string modelId(const json& cfg){static const std::map<std::string,std::string> names={{"기존 SDXL","sdxl"},{"애니 · Illustrious XL","illustrious"},{"문장·실사 · FLUX 빠르게","flux2_klein"},{"문장·실사 · FLUX 고품질","flux2_klein_base"}};auto found=names.find(str(cfg,"workflow_profile","기존 SDXL"));if(found==names.end())throw std::runtime_error("지원하지 않는 생성 모델입니다.");return found->second;}
inline bool modelLocked(const json& cfg,const std::string& key){auto profile=modelGuards().at(modelId(cfg));auto unsupported=profile.at("unsupported");return profile.at("locked").contains(key)||std::find(unsupported.begin(),unsupported.end(),json(key))!=unsupported.end();}
inline void modelSetting(json& cfg,const std::string& key,const json& value){if(key=="hires_enabled")cfg[key]=value.get<bool>()?"켜기":"끄기";else cfg[key]=value.is_string()?value.get<std::string>():value.dump();}
inline void applyModelSettings(json& cfg,bool defaults=false){auto profile=modelGuards().at(modelId(cfg));if(defaults)for(auto& [key,value]:profile.at("defaults").items())modelSetting(cfg,key,value);for(auto& [key,value]:profile.at("locked").items())modelSetting(cfg,key,value);for(auto& key:profile.at("unsupported")){auto name=key.get<std::string>();cfg[name]=name=="lora"?"사용 안 함":name=="lora_strength"||name=="lora_clip_strength"?"1":"서버 기본";}}
inline json guardModelControls(const json& cfg,json value){auto profile=modelGuards().at(modelId(cfg));for(auto& key:profile.at("unsupported"))value.erase(key.get<std::string>());for(auto& [key,required]:profile.at("locked").items())value[key]=required;if(modelId(cfg)=="flux2_klein"&&value.value("hires_steps",0)>0)value["hires_steps"]=4;for(auto& [key,bounds]:profile.at("ranges").items())if(value.contains(key)){double n=value.at(key).get<double>();if(!std::isfinite(n)||n<bounds[0].get<double>()||n>bounds[1].get<double>())throw std::runtime_error("선택한 모델의 설정 범위를 벗어났습니다: "+key);}return value;}
}
