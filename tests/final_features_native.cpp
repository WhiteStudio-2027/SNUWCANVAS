#include "output_layout.hpp"
#include "migration_copy.hpp"
#include "seed_candidates.hpp"
#include <iostream>
using namespace snuw;
void requireTest(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 if(argc!=2)return 2;fs::path root=fs::absolute(wide(argv[1]));auto source=root/"source",metadata=root/"metadata",destination=root/"destination",account=root/"account";
 fs::create_directories(source);fs::create_directories(metadata);fs::create_directories(destination);fs::create_directories(account);
 std::atomic_bool cancel=false;std::string scope(64,'a'),channel(32,'b');
 atomicWrite(source/"one.png","first source bytes");atomicWrite(source/"two.png","second source bytes");atomicWrite(metadata/"one.json",json{{"owner",scope},{"content_mode","standard"}}.dump());
 auto plan=migrationCopyPlan(source,destination,metadata,scope,channel,"safe",cancel);
 requireTest(plan["images"]==2,"Image count differs");requireTest(plan["entries"].size()==3,"JSON sidecar missing");
 auto binding=migrationCopyApply(plan,account,cancel);auto target=fs::path(wide(str(binding,"target")));
 requireTest(readFile(source/"one.png")=="first source bytes","Source changed");
 requireTest(readFile(target/"images/one.png")=="first source bytes","Copy differs");
 auto transaction=str(binding,"transaction");auto history=json::parse(readFile(account/"config/migration-history"/(transaction+".json")));
 atomicWrite(target/"images/two.png","modified copy");atomicWrite(target/"images/new.png","new image");
 auto result=migrationCopyRollback(history,account,scope,channel,cancel);
 requireTest(result["removed"]==2&&result["preserved"]==1,"Rollback ownership/hash policy differs");
 requireTest(fs::exists(source/"one.png")&&fs::exists(metadata/"one.json"),"Rollback deleted source");
 requireTest(readFile(target/"images/two.png")=="modified copy"&&readFile(target/"images/new.png")=="new image","Rollback deleted changed/new file");
 bool denied=false;try{migrationCopyRollback(history,account,std::string(64,'c'),channel,cancel);}catch(...){denied=true;}requireTest(denied,"Foreign rollback allowed");
 denied=false;try{copyRelative(L"../escape.png");}catch(...){denied=true;}requireTest(denied,"Traversal accepted");
 auto candidates=rankSeedCandidates({json{{"candidate_id","b"},{"prompt","quiet forest landscape"},{"stars",5}},json{{"candidate_id","a"},{"prompt","city street at night"},{"stars",4}}},"city street at night");
 requireTest(str(candidates[0],"candidate_id")=="a","Text relevance lost");
 auto config=seedConfig(json{{"seed",1},{"count",3},{"width",832},{"height",1216}});requireTest(!config.contains("seed")&&!config.contains("count"),"Seed compatibility includes seed/count");
 std::cout<<json{{"ok",true},{"copy_json",true},{"original_preserved",true},{"rollback_preserves_new_modified",true},{"scope_denied",true},{"seed_ranking",true}}.dump();return 0;
 }catch(const std::exception& error){std::cerr<<error.what();return 1;}}
