// ROOT specific includes
#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TROOT.h"

// albers specific includes
#include "albers/Reader.h"
#include "albers/Registry.h"
#include "albers/CollectionBase.h"

#include <stdexcept>
#include <glob.h>


namespace albers {

  void* Reader::getBuffer(const unsigned collectionID) {
    void* buffer;
    auto name = m_registry->getNameFromID(collectionID);
    readCollection(name);
    m_registry->lazyGetPODAddressFromID(collectionID, buffer);
    return buffer;
  }

  void Reader::readRegistry(){
    // COLIN: worried about what happens if this function is called twice.
    m_registry = new Registry();
    std::cout<<"read registry "<<m_file<<std::endl;
    TTree* metadatatree = (TTree*)m_file->Get("metadata");
    metadatatree->SetBranchAddress("Registry",&m_registry);
    metadatatree->GetEntry();
    m_registry->setReader(this);
  }

  CollectionBase* Reader::readCollection(const std::string& name) {
    std::cout<<"reading "<<name<<std::endl;
    // has the collection already been constructed?
    auto p = std::find_if(begin(m_inputs), end(m_inputs),
			  [name](Reader::Input t){ return t.second == name;});
    if (p != end(m_inputs)){
      // std::cout<<"collection already there"<<std::endl;
      return p->first;
    }
    TBranch* branch = m_eventTree->GetBranch(name.c_str());
    if(branch == nullptr)
      return nullptr;
    
    CollectionBase* collection = nullptr;
    auto PODname= branch->GetClassName();
    TClass* theClass = gROOT->GetClass(PODname);
    void* buffer = theClass->New();
    //COLIN: I think one might not have to rebuild the transient collections, but just let branch->GetEntry map the data to these collections.
    //now create the transient collections
    // some workaround until gcc supports regex properly:
    auto PODnameString = std::string(PODname);
    auto start = PODnameString.find("<");
    auto end   = PODnameString.find(">");
    auto classname = PODnameString.substr(start+1, end-start-1);
    auto collectionClassName = classname+"Collection";
    TClass* collectionClass = gROOT->GetClass(collectionClassName.c_str());
    collection = static_cast<CollectionBase*>(collectionClass->New());
    // now connect buffer, collection and branch
    collection->setPODsAddress(buffer);
    branch->SetAddress(collection->_getRawBuffer());
    m_inputs.emplace_back(std::make_pair(collection,name));
    // let the registry know about what happened
    m_registry->setPODAddress(name,buffer);
    branch->GetEntry(m_eventNumber);
    collection->prepareAfterRead(m_registry);
    std::cout<<"done "<<name<<std::endl;
    return collection;
  }


  
  void Reader::openFile(const std::string& filename){
    
    glob_t glob_result;
    glob(filename.c_str(),GLOB_TILDE,NULL,&glob_result);
    if(glob_result.gl_pathc>1) {
      TChain* chain = new TChain("events");
      m_eventTree = chain;
      for(unsigned int i=0;i<glob_result.gl_pathc;++i){
	std::cout<<"chain file "<<glob_result.gl_pathv[i]<<std::endl;
        chain->Add(glob_result.gl_pathv[i]);
      }
      m_file = chain->GetFile();
      globfree(&glob_result);
    }
    else {
      m_file = new TFile(glob_result.gl_pathv[0],"READ","data file");
      if(m_file->IsZombie()) {
	throw std::runtime_error( std::string("file ") + filename + " does not exist." );
      }
      m_eventTree = (TTree*) m_file->Get("events");
    }
    readRegistry();
  }

  
  void Reader::readEvent(){
    m_eventTree->GetEntry();
    // and now update all collections
    // if Registry shouldn't be global, we have to do this operation
    for(auto inputs : m_inputs){
      inputs.first->prepareAfterRead(m_registry);
    }
  }

  Reader::~Reader() {
    // delete all collections
    // at the moment it is done in the EventStore;
    // TODO: who deletes the buffers?
  }

  void Reader::endOfEvent() {
    m_eventNumber++;
    m_registry->resetAddresses();
    m_inputs.clear();
  }

  unsigned Reader::getEntries() const {
    return m_eventTree->GetEntries();
  }

  void Reader::goToEvent(unsigned evnum) { 
    m_eventNumber = evnum; 
    m_registry->resetAddresses();
    m_inputs.clear();    
  }


} //namespace
