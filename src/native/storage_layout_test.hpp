#pragma once
namespace snuw {
inline int storageLayoutTest(const fs::path& folder){try{
 auto cfg=defaults(folder);cfg["_local_account_scope"]=std::string(64,'a');cfg["channel_id"]=std::string(32,'b');cfg["creator_name"]="old name";cfg["channel_name"]="same name";
 auto safe=outputDirectory(cfg);auto image=safe/"sample.png";atomicWrite(folder/"generated/.snuw-output.json","{\"schema\":1}");atomicWrite(image,"fixture");auto metadata=imageMetadataPath(image);atomicWrite(metadata,"{}");
 if(safe!=folder/"generated/data"/std::string(32,'a')/std::string(32,'b')/"safe/images"||metadata.parent_path()!=safe.parent_path()/"metadata")throw std::runtime_error("Stable storage path mismatch");
 cfg["creator_name"]="renamed";cfg["channel_name"]="renamed";if(outputDirectory(cfg)!=safe)throw std::runtime_error("Rename changed storage");cfg["content_mode"]="hardcore";auto hc=outputDirectory(cfg)/"sample.png";atomicWrite(hc,"fixture");if(!structuredHardcoreImage(hc)||structuredHardcoreImage(image))throw std::runtime_error("Mode isolation failed");
 cfg["channel_id"]=std::string(32,'c');if(outputDirectory(cfg)==safe)throw std::runtime_error("Channel collision");cfg["channel_id"]="../bad";bool rejected=false;try{outputDirectory(cfg);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("Unsafe ID accepted");
 auto legacy=folder/"generated/User/Standard/Channel/old.png";atomicWrite(legacy,"fixture");if(imageMetadataPath(legacy)!=folder/"generated/_metadata/User/Channel/old.json")throw std::runtime_error("Legacy metadata mapping changed");
 auto targets=galleryDeleteTargets(folder/"generated",image);if(targets.size()!=2||targets[1]!=metadata)throw std::runtime_error("Separate metadata not deleted with image");
 auto migrated=folder/"migrated/user/channel/hardcore";atomicWrite(migrated/".snuw-layout-migration.json",json{{"mode","hardcore"}}.dump());auto nested=migrated/"images/nested/a.png";atomicWrite(nested,"fixture");if(imageMetadataPath(nested)!=migrated/"metadata/nested/a.json"||!structuredHardcoreImage(nested))throw std::runtime_error("Migration metadata mapping failed");
 atomicWrite(folder/"storage-layout-result.json",json{{"ok",true},{"rename_stable",true},{"mode_isolation",true},{"legacy",true},{"metadata_delete",true},{"migration_mapping",true}}.dump());return 0;
 }catch(const std::exception& e){atomicWrite(folder/"storage-layout-result.json",json{{"ok",false},{"error",e.what()}}.dump());return 1;}}
}
