#pragma once
#include "app.hpp"
namespace snuw {
inline int learningDatasetTest(const fs::path& folder,const std::string& base){try{
 auto store=std::make_shared<Store>(folder/"state");store->load();store->useWorkspace(localAccountScope(base,"learning-owner"));App app(store);std::string profile(32,'a');
 store->settings.update({{"api",base},{"api_key","learning-test-key"},{"content_mode","hardcore"},{"profile_target",profile+" | Batch"}});
 std::vector<fs::path> files;for(int i=0;i<100;i++)files.push_back(folder/(std::to_string(i)+".png"));
 auto awaitBatch=[&]{json result;auto until=std::chrono::steady_clock::now()+std::chrono::seconds(80);while(std::chrono::steady_clock::now()<until){for(auto& event:app.engine->take())if(event.text=="reference_batch")result=event.data;if(!result.is_null())break;Sleep(10);}if(result.is_null())throw std::runtime_error("Dataset transfer timed out");if(!str(result,"message").empty())throw std::runtime_error(str(result,"message"));app.learningBusy=false;app.referenceUploadRunning=false;return result;};
 app.startReferenceTransfer(files);auto added=awaitBatch();auto refs=added["reply"].value("references",json::array());if(refs.size()!=100||added.value("done",0)!=100)throw std::runtime_error("Bulk registration lost files");
 bool limited=false;try{app.startReferenceTransfer(std::vector<fs::path>(101,folder/"0.png"));}catch(const std::exception& x){limited=std::string(x.what()).find("100")!=std::string::npos;}if(!limited)throw std::runtime_error("101 image selection was accepted");
 auto asset=str(refs[0],"id");auto thumb=app.fetchReferenceThumbnail(store->settings,store->root,profile,asset,nullptr);if(!fs::exists(thumb)||fs::file_size(thumb)<32)throw std::runtime_error("Reference thumbnail download failed");auto cache=app.referenceCache(profile,asset,false);
 if(localImageVisible(store->settings,cache))throw std::runtime_error("Unregistered reference visible");
 auto newest=str(refs[1],"id");auto newestCache=app.referenceCache(profile,newest,false);
 auto began=std::chrono::steady_clock::now();app.showReferencePreview(profile,asset);Sleep(50);app.showReferencePreview(profile,newest);
 if(app.learningBusy||std::chrono::steady_clock::now()-began>std::chrono::milliseconds(200))throw std::runtime_error("Preview download blocked the UI");
 auto awaitPreview=[&]{auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);while(app.referencePreviewLoading&&std::chrono::steady_clock::now()<deadline){for(auto& event:app.engine->take())if(event.text=="reference_download")app.learningReply(event);Sleep(5);}if(app.referencePreviewLoading)throw std::runtime_error("Preview remained busy");};
 awaitPreview();bool downloaded=app.selected==newestCache&&app.focusPreview;
 if(!downloaded||!localImageVisible(store->settings,newestCache))throw std::runtime_error("Latest downloaded reference not displayed");
 auto other=store->settings;other["_local_account_scope"]="another-account";if(localImageVisible(other,newestCache))throw std::runtime_error("Reference leaked across accounts");
 app.showReferencePreview(profile,std::string(32,'f'));awaitPreview();if(app.referencePreviewError.empty()||app.learningBusy)throw std::runtime_error("Download error left the UI busy");
 app.showReferencePreview(profile,std::string(32,'f'));awaitPreview();if(!app.referencePreviewError.empty()||!app.referencePreviewSelected())throw std::runtime_error("Failed download could not retry");
 std::vector<std::string> ids;for(auto& row:refs)ids.push_back(str(row,"id"));app.startReferenceTransfer({},ids);auto deleted=awaitBatch();if(!deleted["reply"].value("references",json::array()).empty()||deleted.value("done",0)!=100)throw std::runtime_error("Bulk delete did not refresh the list");
 app.engine->stopping=true;app.engine->stop();atomicWrite(folder/"learning-dataset-result.json",json{{"ok",true},{"uploaded",100},{"deleted",100},{"preview_downloaded",downloaded},{"limit_100",limited},{"gui_opened",false}}.dump());return 0;
 }catch(const std::exception& x){atomicWrite(folder/"learning-dataset-result.json",json{{"ok",false},{"error",x.what()},{"gui_opened",false}}.dump());return 1;}}
}
