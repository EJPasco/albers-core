#ifndef REGISTRY_H
#define REGISTRY_H

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

// albers specific includes

/*
The Registry knows about the position of
transient collections and all the buffers
in memory. If some collection/buffer is missing,
it may ask the Reader for help depending on whether
the lookup is 'lazy' or not.

COLIN: when getting a collection, it is in fact the EventStore that asks the Reader for help, not the Registry. Are the lines above referring to something else?

COLIN: it seems the registry also keeps track of the collection IDs, which might play a role in the Writer.
*/

#include <iostream>

namespace albers {

  class Reader;
  class CollectionBase;

  class Registry{

  public:

  Registry():
    m_addresses(), m_collectionIDs(), m_names()
  {};

    // get the pod address but don't do anything else
    template<typename T>
    void lazyGetPODAddressFromID(unsigned ID, T*&) const;

    template<typename T>
    void getPODAddressFromID(unsigned ID, T*&) const;

    template<typename T>
    void getPODAddressFromName(const std::string& name, T*&) const;

    template<typename T>
    void getCollectionFromName(const std::string& name, T*&) const;

    template<typename T>
    unsigned getIDFromPODAddress(T* address) const;

    template<typename T>
    void setPODAddress(unsigned ID, T* address);

    template<typename T>
    void setPODAddress(const std::string& name, T* address);

    std::string getNameFromID(unsigned ID) const {
      std::vector<unsigned>::const_iterator result = std::find(m_collectionIDs.begin(), m_collectionIDs.end(), ID);
      unsigned int index = result - m_collectionIDs.begin();
      return m_names[index];
    };

    template<typename T>
    unsigned registerPOD(T* address, const std::string& name); // returns the ID

    void resetAddresses(){
      for (int i=0, size=m_addresses.size();i<size;++i){
	m_addresses[i] = 0;
      }
    }

    void setReader(Reader* reader) {m_reader = reader;};
    Reader* reader(){return m_reader;};

    std::vector<std::string>& names(){ return m_names;};

    /// Prints collection information
    void print() const;

  private:
    void doGetPODAddressFromID(unsigned ID, void*& address) const;
    std::vector<void*>  m_addresses; //! transient
    std::vector<albers::CollectionBase*>  m_collections; //! transient
    std::vector<unsigned>    m_collectionIDs;
    std::vector<std::string> m_names;
    Reader*                  m_reader; //! transient
  };

#ifndef __GCCXML__
template<typename T>
  void Registry::lazyGetPODAddressFromID(unsigned ID, T*& address) const {
  auto result = std::find(m_collectionIDs.begin(), m_collectionIDs.end(), ID);
  if (result == end(m_collectionIDs)){
    address = 0;
  } else {
    auto index = result - m_collectionIDs.begin();
    address = static_cast<T*>(m_addresses[index]);
  }
}

template<typename T>
void Registry::getPODAddressFromID(unsigned ID, T*& address) const {
  void* tmp;
  doGetPODAddressFromID(ID, tmp);
  address = static_cast<T*>(tmp);
}

template<typename T>
void Registry::getPODAddressFromName(const std::string& name, T*& address) const {
  auto result = std::find(begin(m_names), end(m_names), name);
  if (result == end(m_names)){
    address = nullptr;
  } else {
    auto index = result - m_names.begin();
    address = static_cast<T*>(m_addresses[index]);
  }
}

template<typename T>
void Registry::getCollectionFromName(const std::string& name, T*& collection) const {
  auto result = std::find(begin(m_names), end(m_names), name);
  if (result == end(m_names)){
    collection = nullptr;
  } else {
    auto index = result - m_names.begin();
    collection = static_cast<T*>(m_collections[index]);
  }
}

template<typename T>
unsigned Registry::getIDFromPODAddress(T* address) const {
  auto bare_address = static_cast<void*>(address);
  auto result = std::find(begin(m_addresses), end(m_addresses), bare_address);
  auto index = result - m_addresses.begin();
  return m_collectionIDs[index];
}

template<typename T>
void Registry::setPODAddress(unsigned ID, T* address){
  auto result = std::find(begin(m_collectionIDs), end(m_collectionIDs), ID);
  auto index = result - m_collectionIDs.begin();
  auto bare_address = static_cast<void*>(address);
  m_addresses[index] = bare_address;;
}

template<typename T>
void Registry::setPODAddress(const std::string& name, T* address){
  auto result = std::find(begin(m_names), end(m_names), name);
  auto index = result - m_names.begin();
  auto bare_address = static_cast<void*>(address);
  m_addresses[index] = bare_address;;
}

template<typename T>
unsigned Registry::registerPOD(T* collection, const std::string& name){
  auto bare_address = collection->_getRawBuffer();
  auto result = std::find(begin(m_names), end(m_names), name);
  unsigned ID = 0;
  if (result == m_names.end()) {
      std::hash<std::string> hash;
      m_addresses.emplace_back(bare_address);
      m_names.emplace_back(name);
      m_collections.emplace_back(collection);
      ID = hash(name);
      m_collectionIDs.emplace_back( ID );
      collection->setID(ID);
   } else {
    auto index = result - m_names.begin();
    ID = m_collectionIDs[index];
   }
  return ID;
}

#endif
} //namespace
#endif
