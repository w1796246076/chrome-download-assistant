/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
* This code was based on the npsimple.c sample code in Gecko-sdk.
*
* The contents of this file are subject to the Mozilla Public License Version
* 1.1 (the "License"); you may not use this file except in compliance with
* the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* Contributor(s):
*   Bo Chen   <chen_bo-bj@vanceinfo.com>
*   Jing Zhao <jingzhao@google.com>
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or 
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
* ***** END LICENSE BLOCK ***** */

#include "flashget.h"
#include <afxdisp.h>
#include <string>
#include <IOSTREAM>
#include "../utils.h"

using namespace std;

// Flashget 3
#import "FlashGetHook.dll"
using namespace FlashgetHookLib;

// Flashget old
#import "jccatch.dll"
using namespace FGCATCHURLLib;

// Flashget version
FlashgetVer ver;

// Check if Flashget is enabled.
bool InvokeFlashgetIsEnabled(NPObject* obj, const NPVariant* args,
                             uint32_t argCount, NPVariant* result) {
  result->type = NPVariantType_Bool;
  HRESULT hr = ::CoInitialize(0);
  IIFlashGetNetscape *flashget3;
  hr = Utils::GetCom(__uuidof(IFlashGetNetscape),
                     __uuidof(IIFlashGetNetscape), (void**)&flashget3); 
  if (FAILED(hr)) {
    FGCATCHURLLib::IJetCarNetscape *flashget1;
    hr = Utils::GetCom(__uuidof(FGCATCHURLLib::JetCarNetscape),
                       __uuidof(FGCATCHURLLib::IJetCarNetscape),
                       (void**)&flashget1); 
    if (FAILED(hr)) {
      result->value.intValue = FALSE;
    } else {
      ver = Flashget1;
      result->value.intValue = TRUE;
      flashget1->Release();
    }
  } else {
    ver = Flashget3;
    result->value.intValue = TRUE;
    flashget3->Release();
  }
  return true;
}

// Add a link to download in Flashget.
bool InvokeFlashgetAddLink(NPObject* obj, const NPVariant* args,
                           uint32_t argCount, NPVariant* result) {
  if (argCount == 0) {
    return false;
  } else if (argCount == 3 && NPVARIANT_IS_STRING(args[0]) &&
             NPVARIANT_IS_STRING(args[1]) && NPVARIANT_IS_STRING(args[2])) {
    if (ver == Flashget1) {
      HRESULT hr = ::CoInitialize(0);
      FGCATCHURLLib::IJetCarNetscape *pAgent;
      hr = Utils::GetCom(__uuidof(FGCATCHURLLib::JetCarNetscape), 
                         __uuidof(FGCATCHURLLib::IJetCarNetscape),
                         (void**)&pAgent); 
      if (FAILED(hr)) {
        return false;
      } else {
        hr = pAgent->AddUrl(
            NPVARIANT_TO_STRING(args[0]).UTF8Characters,
            Utils::Utf8ToUnicode(
                (char*)NPVARIANT_TO_STRING(args[1]).UTF8Characters),
            NPVARIANT_TO_STRING(args[2]).UTF8Characters);
        pAgent->Release();
      }
    } else if (ver == Flashget3) {
      HRESULT hr = ::CoInitialize(0);
      IIFlashGetNetscape *pAgent;
      hr = Utils::GetCom(__uuidof(IFlashGetNetscape), 
                         __uuidof(IIFlashGetNetscape), (void**)&pAgent); 
      if (FAILED(hr)) {
        return false;
      } else {
        hr = pAgent->AddUrl(
            NPVARIANT_TO_STRING(args[0]).UTF8Characters,
            Utils::Utf8ToUnicode(
                (char*)NPVARIANT_TO_STRING(args[1]).UTF8Characters),
            NPVARIANT_TO_STRING(args[2]).UTF8Characters);
        pAgent->Release();
      }
    }
  } else {
    return false;
  }
  return true;
}

bool InvokeFlashgetCommit(NPObject* obj, const NPVariant* args,
                          uint32_t argCount, NPVariant* result) {
  // HRESULT hr = pAgent->CommitTasks();
  return true;
}

// Download all links with Flashget.
bool InvokeFlashgetDownloadAll(NPObject* obj, const NPVariant* args,
                               uint32_t argCount, NPVariant* result) {
  bool flag = true;
  if (argCount ==0 ){
    flag = false;
  } else if (NPVARIANT_IS_STRING(args[0]) &&
             NPVARIANT_IS_INT32(args[argCount - 1])) {
    // build array
    SAFEARRAY *psa = Utils::CreateArray(argCount - 1);
    COleVariant var;
    long count = argCount - 1;
    for (long i = 0 ; i < count ; i++) {
      if (NPVARIANT_IS_STRING(args[i])) {
        var.SetString(
            Utils::Utf8ToUnicode(
                (char*)NPVARIANT_TO_STRING(args[i]).UTF8Characters),
            VT_BSTR);
        SafeArrayPutElement(psa, &i, &var);
      } else {
        break;
      }
    }
    VARIANT vList;
    vList.vt = VT_VARIANT|VT_ARRAY|VT_BYREF;
    vList.pparray = &psa;
    if (ver == Flashget1) {
      HRESULT hr = ::CoInitialize(0);
      FGCATCHURLLib::IJetCarNetscape *pAgent;
      hr = Utils::GetCom(__uuidof(FGCATCHURLLib::JetCarNetscape), 
                         __uuidof(FGCATCHURLLib::IJetCarNetscape),
                         (void**)&pAgent); 
      if (FAILED(hr)) {
        flag = false;
      } else {
        pAgent->AddUrlList(&vList);
        pAgent->Release();
      }
    } else if (ver == Flashget3) {
      HRESULT hr = ::CoInitialize(0);
      IIFlashGetNetscape *pAgent;
      hr = Utils::GetCom(__uuidof(IFlashGetNetscape),
                         __uuidof(IIFlashGetNetscape),  (void**)&pAgent); 
      if (FAILED(hr)) {
        flag = false;
      } else {
        pAgent->AddAll(
            &vList, NPVARIANT_TO_STRING(args[0]).UTF8Characters, 
            "FlashGet3", "","0");
        pAgent->Release();
      }
    }
    SafeArrayDestroy(psa);
  } else {
    flag = false;
  }
  return flag;
}