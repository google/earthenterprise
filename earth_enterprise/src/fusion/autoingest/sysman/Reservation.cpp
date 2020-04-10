// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include "Reservation.h"
#include <khResourceManager.h>
#include <khFileUtils.h>
#include <khException.h>


// ****************************************************************************
// ***  CPUReservationImpl
// ****************************************************************************
CPUReservationImpl::CPUReservationImpl(const std::string &host_, unsigned int n) :
    host(host_), num_(n) {

  // theResourceManager already incremented the host's usedCPUs
}

void
CPUReservationImpl::Release(void)
{
  ReservationImpl::Release();

  theResourceManager.ReleaseCPUReservation(host, num_);
}


// ****************************************************************************
// ***  VolumeReservationImpl
// ****************************************************************************
VolumeReservationImpl::VolumeReservationImpl(const std::string &assetpath_,
                                             const khFusionURI &uri_)
    : assetpath(assetpath_), uri(uri_)
{
  // theResourceManager already made the persistent reservation for me

}

void
VolumeReservationImpl::Release(void)
{
  ReservationImpl::Release();

  // drop the reservation's persistence
  theResourceManager.ReleaseVolumeReservation(uri.Volume(), uri.Path());
}


VolumeReservationImpl::~VolumeReservationImpl(void)
{
  if (!released) {
    // I haven't been release before. That's my signal that the task that
    // built this reservation failed one way or another. So we need to
    // clean the files.
    theResourceManager.CleanVolumeReservation(uri.Volume(), uri.Path());

    // Now we release the reservation
    Release();
  }
}

Reservation
VolumeReservationImpl::Make(const std::string &assetpath,
                            const khFusionURI &uri) {
  try {
    return Reservation(khRefGuardFromNew(
        new VolumeReservationImpl(assetpath, uri)));
  } catch (...) {
    return Reservation();
  }
}

bool
VolumeReservationImpl::BindFilename(const std::string &assetp,
                                    std::string &bound) {
  if (assetp == assetpath) {
    bound = uri.NetworkPath();
    return true;
  }
  return false;
}
