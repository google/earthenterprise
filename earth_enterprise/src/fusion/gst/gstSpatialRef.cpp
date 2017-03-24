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


#include <notify.h>
#include <gstSpatialRef.h>

gstSpatialRef::gstSpatialRef(OGRSpatialReference* incoming) {
  if (lat_long_.SetWellKnownGeogCS("WGS84") != OGRERR_NONE)
    notify(NFY_WARN, "Errors creating WGS84 coordinate system");

  if (getNotifyLevel() >= NFY_INFO) {
    char* pszWKT = NULL;
    incoming->exportToPrettyWkt(&pszWKT, false);
    notify(NFY_INFO, "Incoming SRS: \n%s", pszWKT);
    CPLFree(pszWKT);

    pszWKT = NULL;
    lat_long_.exportToPrettyWkt(&pszWKT, false);
    notify(NFY_INFO, "Outgoing SRS: \n%s", pszWKT);
    CPLFree(pszWKT);
  }

  coord_trans_ = OGRCreateCoordinateTransformation(incoming, &lat_long_);

  if (coord_trans_ == NULL) {
    notify(NFY_WARN, "Failed to create transformation");
    status_ = false;
  } else {
    notify(NFY_INFO, "Successfully created transformation");
    status_ = true;
  }

  incoming_ = incoming->Clone();
}

gstSpatialRef::~gstSpatialRef() {
  delete coord_trans_;
}


bool gstSpatialRef::Transform(double* x, double* y, double* z) const {
  static int maxErrorMessages = 20;

  // do nothing if our transformation is invalid
  if (coord_trans_ == NULL)
    return false;

  double ox = *x;
  double oy = *y;
  double oz = *z;

  if (!coord_trans_->Transform(1, &ox, &oy, &oz)) {
    if (maxErrorMessages > 0) {
      notify(NFY_WARN, "Transform failed (%lf,%lf) -> (%lf,%lf)",
             *x, *y, ox, oy);
      maxErrorMessages--;
    } else if (maxErrorMessages == 0) {
      notify(NFY_WARN, "Transform failed (%lf,%lf) -> (%lf,%lf)",
             *x, *y, ox, oy);
      notify(NFY_WARN, "(Suppressing further errors)");
      maxErrorMessages--;
    }
    // if the transform failed, stuff some valid default values
    *x = 0.5;
    *y = 0.5;
    *z = 1.0;
    return false;
  }

  *x = ox;
  *y = oy;
  *z = oz;
  return true;
}
