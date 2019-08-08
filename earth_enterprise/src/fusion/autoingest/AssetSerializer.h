#ifndef ASSET_SERIALIZER_H
#define ASSET_SERIALIZER_H

#include "common/khFileUtils.h"
#include "common/khxml/khxml.h"
#include "common/khxml/khdom.h"
#include "autoingest/.idl/storage/AssetDefs.h"

template<class AssetType>
class AssetSerializerInterface {
  public:
    virtual AssetPointerType<AssetType> Load(const std::string &) = 0;
    virtual bool Save(AssetPointerType<AssetType>, std::string) = 0;
    virtual ~AssetSerializerInterface() = default;
};

template<class AssetType>
class AssetSerializerLocalXML : public AssetSerializerInterface<AssetType>
{
  public:
    virtual AssetPointerType<AssetType> Load(const std::string &boundref)
    {
      return AssetType::Load(boundref);
    }

    virtual bool Save(AssetPointerType<AssetType> asset, std::string filename){
      using AssetStorageType = typename AssetType::Base;
      extern void ToElement(khxml::DOMElement *elem, const AssetStorageType &self);

      std::unique_ptr<GEDocument> doc = CreateEmptyDocument(asset->GetName());
      if (!doc) {
          notify(NFY_WARN,
                "Unable to create empty document: %s", asset->GetName().c_str());
          return false;
      }
      bool status = false;
      try {
          khxml::DOMElement *top = doc->getDocumentElement();
          if (top) {
              const AssetStorageType &storage = *asset;
              ToElement(top, storage);
              asset->SerializeConfig(top);
              status = WriteDocument(doc.get(), filename);

              if (!status && khExists(filename)) {
                  khUnlink(filename);
              }
          } else {
              notify(NFY_WARN, "Unable to create document element %s",
                    filename.c_str());
          }
      } catch (const khxml::XMLException& toCatch) {
          notify(NFY_WARN, "Error saving %s: %s",
                 filename.c_str(), khxml::XMLString::transcode(toCatch.getMessage()));
      } catch (const khxml::DOMException& toCatch) {
          notify(NFY_WARN, "Error saving %s: %s",
                 filename.c_str(), khxml::XMLString::transcode(toCatch.msg));
      } catch (const std::exception &e) {
          notify(NFY_WARN, "%s while saving %s", e.what(), filename.c_str());
      } catch (...) {
          notify(NFY_WARN, "Unable to save %s", filename.c_str());
      }
      return status;
    }
};


#endif //ASSET_SERIALIZER_H
