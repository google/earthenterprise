// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <gdal_priv.h>

#include <gstIconManager.h>
#include <notify.h>
#include <gstMisc.h>
#include <khGuard.h>
#include <khFileUtils.h>
#include <vrtdataset.h>
#include <khConstants.h>
#include <geInstallPaths.h>

gstIcon::gstIcon(const QString& href, IconReference::Type t)
  : config(t, href) {
  // trim extension if given one
  if (khHasExtension(href.latin1(), ".png"))
    config.href.setLength(config.href.length() - 4);
}

gstIconManager* theIconManager = NULL;

void gstIconManager::init(const std::string& path) {
  theIconManager = new gstIconManager(path);
}

gstIconManager::gstIconManager(const std::string& path) {
  SetPath(IconReference::External, path);

  // Find all internal icons.
  if (khDirExists(kGESiteIconsPath))
    SetPath(IconReference::Internal, kGESiteIconsPath);
}

gstIconManager::~gstIconManager() {
  clearIcons(IconReference::Internal);
  clearIcons(IconReference::External);
}

void gstIconManager::clearIcons(IconReference::Type ctype) {
  collection_[ctype].clear();
}

void gstIconManager::SetPath(IconReference::Type ctype,
                             const std::string& path) {
  icon_path_[ctype].resize(0);

  clearIcons(ctype);

  // empty path de-activates directory searches
  if (path.length() == 0)
    return;

  // validate directory
  if (!khDirExists(path)) {
    if (!khMakeDir(path)) {
      notify(NFY_WARN, "Unable to create icon directory: %s", path.c_str());
      return;
    }
    // the icon dir needs wide open permissions for now
    khChmod(path, 0777);
  }

  DIR* icondir = opendir(path.c_str());
  if (icondir == NULL) {
    notify(NFY_WARN, "Unable to open directory: %s", path.c_str());
    return;
  }

  // find all icons in directory
  // This list does not need to be sorted.
  struct dirent *d;
  while ((d = readdir(icondir)) != NULL) {
    // ignore special directory names
    if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
      continue;

    const std::string& name = d->d_name;
    if (Validate(khComposePath(path, name)) == true) {
      collection_[ctype].push_back(gstIcon(name.c_str(), ctype));
    }
  }

  closedir(icondir);

  icon_path_[ctype] = path;
}

const std::string& gstIconManager::GetPath(IconReference::Type ctype) const {
  return icon_path_[ctype];
}

std::string gstIconManager::GetFullPath(IconReference::Type type, int idx) const {
  std::string path = khComposePath(GetPath(type),
                                   collection_[type][idx].href().latin1());
  return khEnsureExtension(path, ".png");
}

bool gstIconManager::Validate(const std::string& path) {
  // ignore any non-png file
  if (!khHasExtension(path, "png") && !khHasExtension(path, "PNG"))
    return false;

  bool valid = false;

  GDALDataset* image = static_cast<GDALDataset*>(
                                           GDALOpen(path.c_str(), GA_ReadOnly));

  if (image != NULL && (image->GetRasterCount() == 4 ||
      (image->GetRasterCount() == 1 &&
      image->GetRasterBand(1)->GetColorInterpretation() == GCI_PaletteIndex))) {
    valid = true;
  }

  delete image;

  return valid;
}

bool gstIconManager::AddIcon(const std::string& path) {
  if (!Validate(path))
    return false;

  std::string base = khReplaceExtension(khBasename(path), ".png");
  std::string outfile = khComposePath(icon_path_[IconReference::External], base);

  if (CopyIcon(path, outfile) == true) {
    collection_[IconReference::External].push_back(gstIcon(
        base.c_str(), IconReference::External));
    return true;
  }

  return false;
}

bool gstIconManager::CopyIcon(const std::string& src_path,
                              const std::string& dst_path) {
  // file must not exist already
  if (khExists(dst_path)) {
    notify(NFY_WARN, "Icon \"%s\" already exists", dst_path.c_str());
    return false;
  }

  GDALDataset* srcDataset = static_cast<GDALDataset*>(
                        GDALOpen(src_path.c_str(), GA_ReadOnly));
  if (!srcDataset) {
    notify(NFY_WARN, "Unable to open icon %s", src_path.c_str());
    return false;
  }

  // determine the image type
  // is it rgb or palette_index type
  bool palette_type = false;
  if (srcDataset->GetRasterCount() == 1 &&
      srcDataset->GetRasterBand(1)->GetColorInterpretation() ==
      GCI_PaletteIndex) {
    palette_type = true;
  } else if (srcDataset->GetRasterCount() != 4) {
    notify(NFY_WARN, "%s: Image type not supported", src_path.c_str());
    return false;
  }

  GDALDataset* oldSrcDataset = 0;
  int target_size = 0;
  bool need_scaling = false;
  int srcXSize = srcDataset->GetRasterXSize();
  int srcYSize = srcDataset->GetRasterYSize();
  if ((srcXSize == 32) || (srcXSize == 64)) {
    target_size = srcXSize;
    if ((srcYSize != srcXSize) &&
        (srcYSize != srcXSize*2) &&
        (srcYSize != srcXSize*3)) {
      need_scaling = true;
    }
  } else if (srcXSize < 32) {
    target_size = 32;
    need_scaling = true;
  } else {
    target_size = 64;
    need_scaling = true;
  }

  if (need_scaling) {
    // create a temp output dataset to scale the src
    // icon to a square target_size*target_size. Later we'll make a stack.
    VRTDataset* tempDataset = new VRTDataset(target_size, target_size);
    int numBands = palette_type ? 1 : 4;
    for (int b = 1; b <= numBands; ++b) {
      tempDataset->AddBand(GDT_Byte, NULL);
      VRTSourcedRasterBand* tempBand =
        static_cast<VRTSourcedRasterBand*>(tempDataset->GetRasterBand(b));

      GDALRasterBand* srcBand = srcDataset->GetRasterBand(b);
      tempBand->AddSimpleSource(srcBand,
                                0, 0, srcXSize, srcYSize,
                                0, 0, target_size, target_size);
      if (palette_type) {
        tempBand->SetColorInterpretation(srcBand->GetColorInterpretation());
        tempBand->SetColorTable(srcBand->GetColorTable());
      }
    }
    oldSrcDataset = srcDataset;
    srcDataset = tempDataset;
    srcXSize = srcYSize = target_size;
  }
  assert(srcXSize == target_size);

  // From here on we assume that we have a square, a stack of 2, or a stack of
  // 3. It will be either 32 or 64 wide. The actual size is stored in srcXSize
  // and srcYSize
  bool simpleCopy = false;
  if (srcYSize == srcXSize * 3)
    simpleCopy = true;

  // create a virtual dataset to represent the desired output image
  VRTDataset* vds = new VRTDataset(target_size, target_size * 3);

  // copy all the bands from the source
  int numBands = palette_type ? 1 : 4;
  for (int b = 1; b <= numBands; ++b) {
    vds->AddBand(GDT_Byte, NULL);
    VRTSourcedRasterBand* vrtBand =
      static_cast<VRTSourcedRasterBand*>(vds->GetRasterBand(b));

    GDALRasterBand* srcBand = srcDataset->GetRasterBand(b);
    if (!simpleCopy) {
      // extract the normal icon (on bottom of input image)
      // and put it on the bottom of new image
      // NOTE: srcYSize calculation lets us hand single, square images
      // as well as two squares stacked on top of each other
      vrtBand->AddSimpleSource(
          srcBand,
          0, srcYSize-target_size, target_size, target_size,
          0, target_size*2, target_size, target_size);

      // extract the highlight icon (on top of input image)
      // and put it in the middle of new image
      vrtBand->AddSimpleSource(srcBand,
                               0, 0, target_size, target_size,
                               0, target_size, target_size, target_size);

      // extract the normal icon (on bottom of input image), scale it to 16x16
      // and put it on the top of the new image
      // NOTE: srcYSize calculation lets us hand single, square images
      // as well as two squares stacked on top of each other
      vrtBand->AddSimpleSource(
          srcBand,
          0, srcYSize-target_size, target_size, target_size,
          0, 0, 16, 16);
    } else {
      vrtBand->AddSimpleSource(srcBand,
                               0, 0, target_size, target_size * 3,
                               0, 0, target_size, target_size * 3);
    }
    if (palette_type) {
      vrtBand->SetColorInterpretation(srcBand->GetColorInterpretation());
      vrtBand->SetColorTable(srcBand->GetColorTable());
    }
  }

  // find output driver
  GDALDriver* pngDriver = GetGDALDriverManager()->GetDriverByName("PNG");
  if (pngDriver == NULL) {
    notify(NFY_FATAL, "Unable to find png driver!");
    return false;
  }

  // write out all bands at once
  GDALDataset* dest = pngDriver->CreateCopy(
                      dst_path.c_str(), vds, false, NULL, NULL, NULL);

  delete dest;
  delete vds;
  delete srcDataset;
  delete oldSrcDataset;

  // just in case the umask trimmed any permissions
  khChmod(dst_path, 0666);

  return true;
}

//
// look for named icon and attempt to remove it
//
bool gstIconManager::DeleteIcon(const std::string& name) {
  bool found = false;

  std::vector<gstIcon>& ext_collection = collection_[IconReference::External];
  for (std::vector<gstIcon>::iterator it = ext_collection.begin();
       it != ext_collection.end(); ++it) {
    if (it->href() == name.c_str()) {
      found = true;
      ext_collection.erase(it);
      break;
    }
  }

  if (!found) {
    notify(NFY_WARN,
           "Unable to find requested icon in master collection: %s", name.c_str());
    return false;
  }

  std::string fullpath = khComposePath(icon_path_[IconReference::External], name);
  fullpath = khEnsureExtension(fullpath, ".png");
  if (!khUnlink(fullpath)) {
    notify(NFY_WARN, "Unable to remove file: %s (%s)", fullpath.c_str(),
           khstrerror(errno).c_str());
    return false;
  }

  return true;
}
