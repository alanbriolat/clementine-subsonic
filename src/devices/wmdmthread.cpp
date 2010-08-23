/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "wmdmthread.h"

#include <mswmdm.h>

#include <QtDebug>

BYTE abPVK[] = {0x00};
BYTE abCert[] = {0x00};


WmdmThread::WmdmThread()
  : device_manager_(NULL)
{
  // Initialise COM
  CoInitialize(0);

  // Authenticate with WMDM
  IComponentAuthenticate* auth;
  if (CoCreateInstance(CLSID_MediaDevMgr, NULL, CLSCTX_ALL,
                       IID_IComponentAuthenticate, (void**) &auth)) {
    qWarning() << "Error creating the IComponentAuthenticate interface";
    return;
  }

  sac_ = CSecureChannelClient_New();
  if (CSecureChannelClient_SetCertificate(
      sac_, SAC_CERT_V1, abCert, sizeof(abCert), abPVK, sizeof(abPVK))) {
    qWarning() << "Error setting SAC certificate";
    return;
  }

  CSecureChannelClient_SetInterface(sac_, auth);
  if (CSecureChannelClient_Authenticate(sac_, SAC_PROTOCOL_V1)) {
    qWarning() << "Error authenticating with SAC";
    return;
  }

  // Create the device manager
  if (auth->QueryInterface(IID_IWMDeviceManager2, (void**)&device_manager_)) {
    qWarning() << "Error creating WMDM device manager";
    return;
  }
}

WmdmThread::~WmdmThread() {
  // Release the device manager
  device_manager_->Release();

  // SAC
  CSecureChannelClient_Free(sac_);

  // Uninitialise COM
  CoUninitialize();
}