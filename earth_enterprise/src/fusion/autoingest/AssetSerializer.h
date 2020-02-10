#ifndef ASSET_SERIALIZER_H
#define ASSET_SERIALIZER_H

#include "common/khFileUtils.h"
#include "common/khxml/khxml.h"
#include "common/khxml/khdom.h"
#include "autoingest/.idl/storage/AssetDefs.h"
#include <khxml/khdom.h>
using namespace khxml;
#include <autoingest/AssetThrowPolicy.h>
#include <autoingest/AssetFactory.h>

template<class AssetType>
class AssetSerializerInterface {
  public:
    virtual AssetPointerType<AssetType> Load(const std::string &) = 0;
    virtual bool Save(AssetPointerType<AssetType>, std::string) = 0;
    virtual ~AssetSerializerInterface() = default;
};

/*
* GetFileInfo and ReadXMLDocument were added as function templates to allow
* them to be overridden in unit tests. The versions here should be used for
* all asset types during normal operation.
*/
template<class AssetType>
bool GetFileInfo(const std::string &fname, uint64 &size, time_t &mtime) {
  return khGetFileInfo(fname, size, mtime);
}

template<class AssetType>
std::unique_ptr<GEDocument> ReadXMLDocument(const std::string &filename) {
  return ReadDocument(filename);
}

template<class AssetType>
class AssetSerializerLocalXML : public AssetSerializerInterface<AssetType>
{
  public:
    virtual AssetPointerType<AssetType> Load(const std::string &boundref)
    {
      std::string filename = AssetType::XMLFilename(boundref);
      AssetPointerType<AssetType> result;
      time_t timestamp = 0;
      uint64 filesize = 0;

      if (GetFileInfo<AssetType>(filename, filesize, timestamp) && (filesize > 0)) {
        std::unique_ptr<GEDocument> doc = ReadXMLDocument<AssetType>(filename);
        if (doc) {
          try {
            DOMElement *top = doc->getDocumentElement();
            if (!top)
              throw khException(kh::tr("No document element"));
            std::string tagname = FromXMLStr(top->getTagName());
            result = AssetFactory::CreateNewFromDOM<AssetType>(tagname,top);
            if (nullptr == result) {
              AssetThrowPolicy::WarnOrThrow
                (kh::tr("Unknown asset type '%1' while parsing %2")
                .arg(ToQString(tagname), ToQString(filename)));
            }
          } catch (const std::exception &e) {
          AssetThrowPolicy::WarnOrThrow
            (kh::tr("Error loading %1: %2")
            .arg(ToQString(filename), QString::fromUtf8(e.what())));
          } catch (...) {
              AssetThrowPolicy::WarnOrThrow(kh::tr("Unable to load ")
                    + filename);
          }
        } else {
            AssetThrowPolicy::WarnOrThrow(kh::tr("Unable to read ")
                  + filename);
        }
      } else {
          AssetThrowPolicy::WarnOrThrow(kh::tr("No such file: ") + filename);
      }

      if (!result) {
        notify(NFY_DEBUG, "Creating placeholder for bad asset %s",
              boundref.c_str());
        // Use SourceAssetImpl since AssetImpl is pure virtual
        result = AssetFactory::CreateNewInvalid<AssetType>(
          AssetType::PLACEHOLDER_ASSET_REGISTRY_KEY, boundref
        );

        // Leave timestamp alone if it failed but there was a valid timestamp
        // we want to remember the timestamp of the one that failed
      }

      // Store the timestamp so the cache can check it later
      result->timestamp = timestamp;
      result->filesize  = filesize;

      return result;
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
