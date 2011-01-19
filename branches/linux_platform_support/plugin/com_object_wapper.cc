#include "stdafx.h"

#include <comdef.h>

#include "com_object_wapper.h"
#include "log.h"

extern Log g_Log;

ComObjectWapper::ComObjectWapper(void) {
  disp_pointer_ = NULL;
}

ComObjectWapper::~ComObjectWapper(void) {
  if (disp_pointer_ != NULL)
    disp_pointer_->Release();
}

NPObject* ComObjectWapper::Allocate(NPP npp, NPClass *aClass) {
  ComObjectWapper* pRet = new ComObjectWapper;
  char logs[256];
  sprintf(logs, "ComObjectWapper this=%ld", pRet);
  g_Log.WriteLog("Allocate", logs);
  if (pRet != NULL) {
    pRet->set_plugin((PluginBase*)npp->pdata);
  }
  return pRet;
}

void ComObjectWapper::Deallocate() {
  char logs[256];
  sprintf(logs, "ComObjectWapper this=%ld", this);
  g_Log.WriteLog("Deallocate", logs);
  delete this;
}

bool ComObjectWapper::HasMethod(NPIdentifier name) {
  bool bRet = false;
  char* szName = NPN_UTF8FromIdentifier(name);
  TCHAR szWName[256];
  TCHAR* p = szWName;
  MultiByteToWideChar(CP_UTF8, 0, szName, -1, szWName, 256);
  DISPID dispid;
  HRESULT hr = disp_pointer_->GetIDsOfNames(IID_NULL, &p, 1,
                                            LOCALE_USER_DEFAULT, &dispid);
  if (SUCCEEDED(hr))
    bRet = true;
  if (szName != NULL)
    NPN_MemFree(szName);
  return bRet;
}

bool ComObjectWapper::Invoke(NPIdentifier name, const NPVariant *args,
                             uint32_t argCount, NPVariant *result) {
  bool bRet = false;
  char szLog[256];
  char* szName = NPN_UTF8FromIdentifier(name);
  g_Log.WriteLog("Invoke", szName);
  TCHAR szWName[256];
  TCHAR* p = szWName;
  MultiByteToWideChar(CP_UTF8, 0, szName, -1, szWName, 256);
  DISPID dispid;
  HRESULT hr = disp_pointer_->GetIDsOfNames(IID_NULL, &p, 1,
                                            LOCALE_USER_DEFAULT, &dispid);
  if (SUCCEEDED(hr)) {
    sprintf_s(szLog, "dispid=%ld", dispid);
    g_Log.WriteLog("Invoke", szLog);
    VARIANT varRet;
    DISPPARAMS params;
    params.cNamedArgs = 0;
    params.rgdispidNamedArgs = 0;
    params.cArgs = argCount;
    VARIANT* varlist = new VARIANT[argCount];
    _bstr_t* bstrList = new _bstr_t[argCount];
    for (int i = 0; i < argCount; i++) {
      switch (args[i].type) {
        case NPVariantType_Bool:
          varlist[argCount-i-1].vt = VT_BOOL;
          varlist[argCount-i-1].boolVal = NPVARIANT_TO_BOOLEAN(args[i]);
          break;
        case NPVariantType_Int32:
          varlist[argCount-i-1].vt = VT_INT;
          varlist[argCount-i-1].intVal = NPVARIANT_TO_INT32(args[i]);
          break;
        case NPVariantType_Double:
          varlist[argCount-i-1].vt = VT_R8;
          varlist[argCount-i-1].dblVal = NPVARIANT_TO_DOUBLE(args[i]);
          break;
        case NPVariantType_String:
          {
            g_Log.WriteLog("Param", NPVARIANT_TO_STRING(args[i]).UTF8Characters);
            bstrList[i] = NPVARIANT_TO_STRING(args[i]).UTF8Characters;
            varlist[argCount-i-1].vt = VT_BSTR;
            int nLen = bstrList[i].length() + 1;
            if (!MultiByteToWideChar(CP_UTF8, 0,
                NPVARIANT_TO_STRING(args[i]).UTF8Characters, -1,
                bstrList[i],nLen)) {
                sprintf(szLog, "GetLastError=%ld", GetLastError());
                g_Log.WriteLog("Error", szLog);
            }
            varlist[argCount-i-1].bstrVal = bstrList[i];
          }
          break;
        case NPVariantType_Object:
          {
            NPObject* pObject = NPVARIANT_TO_OBJECT(args[i]);
            NPIdentifier id = NPN_GetStringIdentifier("length");
            if (id) {
              NPVariant ret;
              if (NPN_GetProperty(get_plugin()->get_npp(), pObject, id, &ret) &&
                  (NPVARIANT_IS_INT32(ret) || NPVARIANT_IS_DOUBLE(ret))) {
                int array_len = NPVARIANT_IS_INT32(ret) ? 
                    NPVARIANT_TO_INT32(ret) : NPVARIANT_TO_DOUBLE(ret);
                sprintf(szLog, "array_len=%d", array_len);
                g_Log.WriteLog("Array", szLog);
                SAFEARRAYBOUND rgsabound[1];
                rgsabound[0].lLbound = 0;
                rgsabound[0].cElements = array_len;
                SAFEARRAY* psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
                if (psa) {
                  for (int index = 0; index < array_len; index++) {
                    id = NPN_GetIntIdentifier(index);
                    NPN_GetProperty(get_plugin()->get_npp(), pObject, id, &ret);
                    if (!NPVARIANT_IS_STRING(ret))
                      continue;
                    const char* array_item = NPVARIANT_TO_STRING(ret).UTF8Characters;
                    g_Log.WriteLog("array", array_item);
                    _bstr_t bstr = array_item;
                    MultiByteToWideChar(CP_UTF8, 0, array_item, -1,
                        bstr, bstr.length()+1);
                    VARIANT var_out;
                    var_out.vt = VT_BSTR;
                    var_out.bstrVal = bstr;
                    SafeArrayPutElement(psa, (long*)&index, &var_out);
                  }
                  varlist[argCount-i-1].vt = VT_ARRAY | VT_VARIANT | VT_BYREF;
                  varlist[argCount-i-1].pparray = new SAFEARRAY*;
                  *varlist[argCount-i-1].pparray = psa;
                }
              }
            }
          }
          break;
        default:
          varlist[argCount-i-1].vt = VT_EMPTY;
          break;
      }
    }
    params.rgvarg = varlist;
    g_Log.WriteLog("Invoke", "Before Invoke");
    unsigned int nErrIndex = 0;

    hr = disp_pointer_->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT,
                               DISPATCH_METHOD, &params, &varRet, NULL,
                               &nErrIndex);
    sprintf_s(szLog,
        "Invoke End,hr=0x%X,GetLastError=%ld,nErrIndex=%ld,varRet.Type=%ld",
        hr, GetLastError(), nErrIndex, varRet.vt);
    g_Log.WriteLog("Invoke", szLog);
    for (int i = 0; i < argCount; i++) {
      if (varlist[i].vt == (VT_ARRAY | VT_VARIANT | VT_BYREF)) {
        SafeArrayDestroy(*varlist[i].pparray);
        delete varlist[i].pparray;
      }
    }
    delete[] varlist;
    delete[] bstrList;
    if (SUCCEEDED(hr)) {
      bRet = true;
      switch(varRet.vt) {
        case VT_BOOL:
          BOOLEAN_TO_NPVARIANT(varRet.boolVal, *result);
          break;
        case VT_INT:
        case VT_I4:
          INT32_TO_NPVARIANT(varRet.intVal, *result);
          break;
        case VT_R8:
          DOUBLE_TO_NPVARIANT(varRet.dblVal, *result);
          break;
        case VT_BSTR:
          _bstr_t bstr = varRet.bstrVal;
          int nLen = bstr.length();
          char* p = (char*)NPN_MemAlloc(nLen*4);
          WideCharToMultiByte(CP_UTF8, 0, bstr, -1, p, nLen*4, 0, 0);
          STRINGZ_TO_NPVARIANT(p, *result);
          break;
      }
    }
  }
  if (szName != NULL)
    NPN_MemFree(szName);
  return bRet;
}

bool ComObjectWapper::HasProperty(NPIdentifier name) {
  bool has_property = ScriptObjectBase::HasProperty(name);
  bool has_method = ScriptObjectBase::HasMethod(name);
  const char* method_name = NPN_UTF8FromIdentifier(name);
  if (!has_property && !has_method) {
    has_property = FindFunctionByInvokeKind(method_name,
        INVOKE_PROPERTYGET | INVOKE_PROPERTYPUT);
    if (has_property) {
      Property_Item item;
      item.property_name = method_name;
      VOID_TO_NPVARIANT(item.value);
      AddProperty(item);
    }
    
    has_method = FindFunctionByInvokeKind(method_name, INVOKE_FUNC);
    if (has_method) {
      Function_Item item;
      item.function_name = method_name;
      item.function_pointer = NULL;
      AddFunction(item);
    }
  }

  return has_property;
}

bool ComObjectWapper::GetProperty(NPIdentifier name, NPVariant *result) {
  bool bRet = false;
  char szLog[256];
  char* szName = NPN_UTF8FromIdentifier(name);
  g_Log.WriteLog("GetProperty", szName);
  TCHAR szWName[256];
  TCHAR* p = szWName;
  MultiByteToWideChar(CP_UTF8, 0, szName, -1, szWName, 256);
  DISPID dispid;
  HRESULT hr = disp_pointer_->GetIDsOfNames(IID_NULL, &p, 1,
                                            LOCALE_USER_DEFAULT, &dispid);
  if (SUCCEEDED(hr)) {
    sprintf_s(szLog, "dispid=%ld", dispid);
    g_Log.WriteLog("GetProperty", szLog);
    VARIANT varRet;
    DISPPARAMS params = {0};
    g_Log.WriteLog("GetProperty", "Before Invoke");
    unsigned int nErrIndex = 0;

    hr = disp_pointer_->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT,
                               DISPATCH_PROPERTYGET, &params, &varRet, NULL,
                               &nErrIndex);
    sprintf(szLog,
            "Invoke End,hr=0x%X,GetLastError=%ld,nErrIndex=%ld,varRet.Type=%ld",
            hr, GetLastError(), nErrIndex, varRet.vt);
    g_Log.WriteLog("GetProperty", szLog);
    if (SUCCEEDED(hr)) {
      bRet = true;
      switch(varRet.vt) {
        case VT_BOOL:
          BOOLEAN_TO_NPVARIANT(varRet.boolVal, *result);
          break;
        case VT_INT:
        case VT_I4:
          INT32_TO_NPVARIANT(varRet.intVal, *result);
          break;
        case VT_R8:
          DOUBLE_TO_NPVARIANT(varRet.dblVal, *result);
          break;
        case VT_BSTR:
          _bstr_t bstr = varRet.bstrVal;
          int nLen = bstr.length();
          char* p = (char*)NPN_MemAlloc(nLen*4);
          WideCharToMultiByte(CP_UTF8, 0, bstr, -1, p, nLen*4, 0, 0);
          STRINGZ_TO_NPVARIANT(p, *result);
          break;
      }
    }
  }
  if (szName != NULL)
    NPN_MemFree(szName);
  return bRet;
}

bool ComObjectWapper::SetProperty(NPIdentifier name,
                                  const NPVariant *value) {
  bool bRet = false;
  char szLog[256];
  char* szName = NPN_UTF8FromIdentifier(name);
  g_Log.WriteLog("SetProperty", szName);
  TCHAR szWName[256];
  TCHAR* p = szWName;
  MultiByteToWideChar(CP_UTF8, 0, szName, -1, szWName, 256);
  DISPID dispid;
  HRESULT hr = disp_pointer_->GetIDsOfNames(IID_NULL, &p, 1,
                                            LOCALE_USER_DEFAULT, &dispid);
  if (SUCCEEDED(hr)) {
    sprintf_s(szLog, "dispid=%ld", dispid);
    g_Log.WriteLog("SetProperty", szLog);
    VARIANT varRet;
    DISPPARAMS params = {0};
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    params.cNamedArgs = 1;
    params.rgdispidNamedArgs = &dispidNamed;
    params.cArgs = 1;
    VARIANT varparam;
    _bstr_t bstr;
    switch (value->type) {
      case NPVariantType_Bool:
        varparam.vt = VT_BOOL;
        varparam.boolVal = NPVARIANT_TO_BOOLEAN(*value);
        break;
      case NPVariantType_Int32:
        varparam.vt = VT_INT;
        varparam.intVal = NPVARIANT_TO_INT32(*value);
        break;
      case NPVariantType_Double:
        varparam.vt = VT_R8;
        varparam.dblVal = NPVARIANT_TO_DOUBLE(*value);
        break;
      case NPVariantType_String:
        {
          g_Log.WriteLog("Param", NPVARIANT_TO_STRING(*value).UTF8Characters);
          bstr = NPVARIANT_TO_STRING(*value).UTF8Characters;
          varparam.vt = VT_BSTR;
          int nLen = bstr.length() + 1;
          if (!MultiByteToWideChar(CP_UTF8, 0,
                                   NPVARIANT_TO_STRING(*value).UTF8Characters, -1,
                                   bstr, nLen)) {
              sprintf(szLog, "GetLastError=%ld", GetLastError());
              g_Log.WriteLog("Error", szLog);
          }
          varparam.bstrVal = bstr;
        }
        break;
      default:
        varparam.vt = VT_EMPTY;
        break;
    }
    params.rgvarg = &varparam;
    g_Log.WriteLog("SetProperty", "Before Invoke");
    unsigned int nErrIndex = 0;

    hr = disp_pointer_->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT,
                               DISPATCH_PROPERTYPUT, &params, &varRet, NULL,
                               &nErrIndex);
    sprintf(szLog,
            "Invoke End,hr=0x%X,GetLastError=%ld,nErrIndex=%ld,varRet.Type=%ld",
            hr, GetLastError(), nErrIndex, varRet.vt);
    g_Log.WriteLog("SetProperty", szLog);
  }
  if (szName != NULL)
    NPN_MemFree(szName);
  return bRet;
}

bool ComObjectWapper::RemoveProperty(NPIdentifier name) {
  bool bRet = false;
  return bRet;
}

bool ComObjectWapper::FindFunctionByInvokeKind(const char* name, 
                                               int invokekind) {
  char szLog[256];
  bool ret = false;
  g_Log.WriteLog("FindFunction", name);
  TCHAR szWName[256];
  TCHAR* p = szWName;
  MultiByteToWideChar(CP_UTF8, 0, name, -1, szWName, 256);
  DISPID dispid;
  HRESULT hr = disp_pointer_->GetIDsOfNames(IID_NULL, &p, 1,
                                            LOCALE_USER_DEFAULT, &dispid);
  if (SUCCEEDED(hr)) {
    sprintf_s(szLog, "dispid=%ld", dispid);
    g_Log.WriteLog("FindFunction", szLog);
    ITypeInfo* typeinfo;
    hr = disp_pointer_->GetTypeInfo(0, LOCALE_USER_DEFAULT, &typeinfo);
    if (SUCCEEDED(hr)) {
      TYPEATTR* attr;
      FUNCDESC* fun;
      hr = typeinfo->GetTypeAttr(&attr);
      if (SUCCEEDED(hr)) {
        for (int index = 0; index < attr->cFuncs; index++) {
          hr = typeinfo->GetFuncDesc(index, &fun);
          if (FAILED(hr))
            break;
          if (fun->memid == dispid && (fun->invkind & invokekind)) {
            typeinfo->ReleaseFuncDesc(fun);
            ret = true;
            break;
          } else
            typeinfo->ReleaseFuncDesc(fun);
        }
        typeinfo->ReleaseTypeAttr(attr);
      }
    }
  }
  return ret;
}