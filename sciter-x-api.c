#include <sciter-x.h>
#include <string.h>

typedef ISciterAPI* (SCAPI *SciterAPI_ptr)();

const char * SCITER_DLL_PATH = SCITER_DLL_NAME;

// getting ISciterAPI reference:

#if defined(STATIC_LIB) || defined(SCITER_BUILD)

    EXTERN_C ISciterAPI* SCAPI SciterAPI();

    ISciterAPI* _SAPI(ISciterAPI* ext) {
       static ISciterAPI* _api = NULL;
       if( ext ) _api = ext;
       if( !_api )
       {
          _api = SciterAPI();
#if defined(__cplusplus) && !defined(PLAIN_API_ONLY)
//          tiscript::ni(_api->TIScriptAPI());
#endif
       }
       assert(_api);
       return _api;
    }

#elif defined(WINDOWS)

    ISciterAPI* _SAPI( ISciterAPI* ext ) {
       static ISciterAPI* _api = NULL;
       if( ext ) _api = ext;
       if( !_api )
       {
          HMODULE hm = LoadLibrary( TEXT(SCITER_DLL_NAME) );
          if(hm) {
            SciterAPI_ptr sciterAPI = (SciterAPI_ptr) GetProcAddress(hm, "SciterAPI");
            if( sciterAPI ) {
              _api = sciterAPI();
//#if defined(__cplusplus) && !defined(PLAIN_API_ONLY)
//              tiscript::ni( _api->TIScriptAPI() );
//#endif
            } else {
              FreeLibrary(hm);
            }

          }
       }
       assert(_api);
       if( !_api ) {
         //::MessageBox(NULL, TEXT("Sciter engine not found, quiting"),TEXT("Error"),MB_OK);
         exit(-1);
       }
       return _api;
    }

#elif defined(OSX)

    //typedef ISciterAPI* SCAPI (*SciterAPI_ptr)();

    ISciterAPI* _SAPI( ISciterAPI* ext )
    {
        static ISciterAPI* _api = NULL;
        if( ext ) _api = ext;
        if( !_api ) {
            pid_t pid;
            char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
            char folderpath[PROC_PIDPATHINFO_MAXSIZE];
            pid = getpid();
            proc_pidpath (pid, pathbuf, sizeof(pathbuf));
            realpath(pathbuf, folderpath);
            *strrchr(folderpath, '/') = '\0';

            void* lib_sciter_handle = dlopen(SCITER_DLL_NAME, RTLD_LOCAL|RTLD_LAZY);
            if( !lib_sciter_handle ) {
                const char* lookup_paths[] =
                {
                    "/" SCITER_DLL_NAME,
                    "/../Frameworks/" SCITER_DLL_NAME, // - (bundle folder)/Contents/Frameworks/
                    "/../../../" SCITER_DLL_NAME // aside of bundle folder (SDK samples)
                };
                for( int n = 0; !lib_sciter_handle && n < 3; ++n ) {
                    char tpath[PROC_PIDPATHINFO_MAXSIZE];
                    strcpy(tpath,folderpath);
                    strcat(tpath, lookup_paths[n]);
                    lib_sciter_handle = dlopen(tpath, RTLD_LOCAL|RTLD_LAZY);
                }
            }
            if (!lib_sciter_handle) {
                fprintf(stderr, "[%s] Unable to load library: %s\n", __FILE__, dlerror());
                exit(EXIT_FAILURE);
            }

            SciterAPI_ptr sapi = (SciterAPI_ptr) dlsym(lib_sciter_handle, "SciterAPI");
            if (!sapi) {
                fprintf(stderr,"[%s] Unable to get symbol: %s\n", __FILE__, dlerror());
                exit(EXIT_FAILURE);
            }
            _api = sapi();
            //tiscript::ni( _api->TIScriptAPI() );
        }
        assert(_api);
        return _api;
    }

#elif defined(ANDROID)

#if 1
    EXTERN_C ISciterAPI *SciterAPI();

    ISciterAPI* _SAPI( ISciterAPI* ext )
    {
        static ISciterAPI* _api = NULL;
        if( ext )
          _api = ext;
        else
          _api = SciterAPI();
        assert(_api);
        //if (!sapi) {
        //  fprintf(stderr, "[%s] Unable to get symbol: %s\n", __FILE__, dlerror());
        //  exit(EXIT_FAILURE);
        //}
        return _api;
    }

#else
    ISciterAPI *_SAPI(ISciterAPI *ext) {
      static ISciterAPI *_api = NULL;
      if (ext) _api = ext;
      if (!_api) {
        void *lib_sciter_handle = NULL;
        // 1. try to load from the same folder as this executable
        lib_sciter_handle = dlopen(SCITER_DLL_NAME, RTLD_LOCAL | RTLD_LAZY);
        if (!lib_sciter_handle) {
          fprintf(stderr, "[%s] Unable to load library: %s\n", dlerror(), SCITER_DLL_NAME);
        }

        SciterAPI_ptr sapi = (SciterAPI_ptr)dlsym(lib_sciter_handle, "SciterAPI");
        if (!sapi) {
          fprintf(stderr, "[%s] Unable to get symbol: %s\n", __FILE__, dlerror());
          exit(EXIT_FAILURE);
        }
        _api = sapi();
      }
      assert(_api);
      return _api;
}

#endif

#elif defined(LINUX)

#include <libgen.h>
#include <dlfcn.h>

// typedef ISciterAPI* SCAPI (*SciterAPI_ptr)();

ISciterAPI *_SAPI(ISciterAPI *ext) {
  static ISciterAPI *_api = NULL;
  if (ext) _api = ext;
  if (!_api) {

    char folderpath[2048] = {0};

    if (readlink("/proc/self/exe", folderpath, sizeof(folderpath)) != -1) {
      dirname(folderpath);
      // strcat  (pathbuf, "/");
    }

    void *lib_sciter_handle = NULL;

    {
      // 1. try to load from the same folder as this executable
      const char *lookup_paths[] = {"/" SCITER_DLL_NAME, NULL};
      for (int n = 0; !lib_sciter_handle; ++n) {
        if (!lookup_paths[n]) break;
        char tpath[2048] = {0};
        strcpy(tpath, folderpath);
        strcat(tpath, lookup_paths[n]);
        // fprintf(stdout, "[%s]\n",tpath);
        lib_sciter_handle = dlopen(tpath, RTLD_LOCAL | RTLD_LAZY);
        if (!lib_sciter_handle) {
          fprintf(stderr, "[%s] Unable to load library: %s\n", dlerror(),
                  tpath);
        }
      }
    }

    if (!lib_sciter_handle) // 2. no luck, try to load from system paths
      lib_sciter_handle = dlopen(SCITER_DLL_NAME, RTLD_LOCAL | RTLD_LAZY);

    if (!lib_sciter_handle) {
      fprintf(stderr, "[%s] Unable to load library: %s\n", __FILE__, dlerror());
      exit(EXIT_FAILURE);
    }

    SciterAPI_ptr sapi = (SciterAPI_ptr)dlsym(lib_sciter_handle, "SciterAPI");
    if (!sapi) {
      fprintf(stderr, "[%s] Unable to get symbol: %s\n", __FILE__, dlerror());
      exit(EXIT_FAILURE);
    }
    _api = sapi();
  }
  assert(_api);
  return _api;
}

#endif


  ISciterAPI* SAPI() {
#if defined(__cplusplus) && !defined(PLAIN_API_ONLY)
    static ISciterAPI* _api = _SAPI(NULL);
#else 
    static ISciterAPI* _api = NULL;
    if (!_api) _api = _SAPI(NULL);
#endif
    return _api;
  }

  LPSciterGraphicsAPI gapi()
  {
    return SAPI()->GetSciterGraphicsAPI();
  }

  LPSciterRequestAPI rapi()
  {
    return SAPI()->GetSciterRequestAPI();
  }


  // defining "official" API functions:

  inline   LPCWSTR SCAPI SciterClassName (void) { return SAPI()->SciterClassName(); }
  inline   UINT    SCAPI SciterVersion (SBOOL major) { return SAPI()->SciterVersion (major); }
  inline   SBOOL    SCAPI SciterDataReady (HWINDOW hwnd,LPCWSTR uri,LPCBYTE data, UINT dataLength) { return SAPI()->SciterDataReady (hwnd,uri,data,dataLength); }
  inline   SBOOL    SCAPI SciterDataReadyAsync (HWINDOW hwnd,LPCWSTR uri, LPCBYTE data, UINT dataLength, LPVOID requestId) { return SAPI()->SciterDataReadyAsync (hwnd,uri, data, dataLength, requestId); }
#if defined(WINDOWS) && !defined(WINDOWLESS)
  inline   LRESULT SCAPI SciterProc (HWINDOW hwnd, UINT msg, WPARAM wParam, LPARAM lParam) { return SAPI()->SciterProc (hwnd,msg,wParam,lParam); }
  inline   LRESULT SCAPI SciterProcND (HWINDOW hwnd, UINT msg, WPARAM wParam, LPARAM lParam, SBOOL* pbHandled) { return SAPI()->SciterProcND (hwnd,msg,wParam,lParam,pbHandled); }
#endif
  inline   SBOOL    SCAPI SciterLoadFile (HWINDOW hWndSciter, LPCWSTR filename) { return SAPI()->SciterLoadFile (hWndSciter,filename); }
  inline   SBOOL    SCAPI SciterLoadHtml (HWINDOW hWndSciter, LPCBYTE html, UINT htmlSize, LPCWSTR baseUrl) { return SAPI()->SciterLoadHtml (hWndSciter,html,htmlSize,baseUrl); }
  inline   VOID    SCAPI SciterSetCallback (HWINDOW hWndSciter, LPSciterHostCallback cb, LPVOID cbParam) { SAPI()->SciterSetCallback (hWndSciter,cb,cbParam); }
  inline   SBOOL    SCAPI SciterSetMasterCSS (LPCBYTE utf8, UINT numBytes) { return SAPI()->SciterSetMasterCSS (utf8,numBytes); }
  inline   SBOOL    SCAPI SciterAppendMasterCSS (LPCBYTE utf8, UINT numBytes) { return SAPI()->SciterAppendMasterCSS (utf8,numBytes); }
  inline   SBOOL    SCAPI SciterSetCSS (HWINDOW hWndSciter, LPCBYTE utf8, UINT numBytes, LPCWSTR baseUrl, LPCWSTR mediaType) { return SAPI()->SciterSetCSS (hWndSciter,utf8,numBytes,baseUrl,mediaType); }
  inline   SBOOL    SCAPI SciterSetMediaType (HWINDOW hWndSciter, LPCWSTR mediaType) { return SAPI()->SciterSetMediaType (hWndSciter,mediaType); }
  inline   SBOOL    SCAPI SciterSetMediaVars (HWINDOW hWndSciter, const SCITER_VALUE *mediaVars) { return SAPI()->SciterSetMediaVars (hWndSciter,mediaVars); }
  inline   UINT    SCAPI SciterGetMinWidth (HWINDOW hWndSciter) { return SAPI()->SciterGetMinWidth (hWndSciter); }
  inline   UINT    SCAPI SciterGetMinHeight (HWINDOW hWndSciter, UINT width) { return SAPI()->SciterGetMinHeight (hWndSciter,width); }
  inline   SBOOL    SCAPI SciterCall (HWINDOW hWnd, LPCSTR functionName, UINT argc, const SCITER_VALUE* argv, SCITER_VALUE* retval) { return SAPI()->SciterCall (hWnd,functionName, argc,argv,retval); }
  inline   SBOOL    SCAPI SciterEval ( HWINDOW hwnd, LPCWSTR script, UINT scriptLength, SCITER_VALUE* pretval) { return SAPI()->SciterEval ( hwnd, script, scriptLength, pretval); }
  inline   VOID    SCAPI SciterUpdateWindow(HWINDOW hwnd) { SAPI()->SciterUpdateWindow(hwnd); }
#if defined(WINDOWS) && !defined(WINDOWLESS)
  inline   SBOOL    SCAPI SciterTranslateMessage (MSG* lpMsg) { return SAPI()->SciterTranslateMessage (lpMsg); }
#endif
  inline  SBOOL    SCAPI SciterSetOption (HWINDOW hWnd, UINT option, UINT_PTR value ) { return SAPI()->SciterSetOption (hWnd,option,value ); }
  inline  VOID    SCAPI SciterGetPPI (HWINDOW hWndSciter, UINT* px, UINT* py) { SAPI()->SciterGetPPI (hWndSciter,px,py); }
  inline  SBOOL    SCAPI SciterGetViewExpando ( HWINDOW hwnd, VALUE* pval ) { return SAPI()->SciterGetViewExpando ( hwnd, pval ); }
#if defined(WINDOWS) && !defined(WINDOWLESS)
  inline  SBOOL    SCAPI SciterRenderD2D (HWINDOW hWndSciter, IUnknown* /*ID2D1RenderTarget**/ prt) { return SAPI()->SciterRenderD2D (hWndSciter,prt); }
  inline  SBOOL    SCAPI SciterD2DFactory (IUnknown** /*ID2D1Factory ***/ ppf) { return SAPI()->SciterD2DFactory (ppf); }
  inline  SBOOL    SCAPI SciterDWFactory (IUnknown** /*IDWriteFactory ***/ ppf) { return SAPI()->SciterDWFactory (ppf); }
#endif
  inline  SBOOL    SCAPI SciterGraphicsCaps (LPUINT pcaps) { return SAPI()->SciterGraphicsCaps (pcaps); }
  inline  SBOOL    SCAPI SciterSetHomeURL (HWINDOW hWndSciter, LPCWSTR baseUrl) { return SAPI()->SciterSetHomeURL (hWndSciter,baseUrl); }
#if defined(OSX) && !defined(WINDOWLESS)
  inline  HWINDOW SCAPI SciterCreateNSView ( LPRECT frame ) { return SAPI()->SciterCreateNSView ( frame ); }
#endif
#if !defined(WINDOWLESS)
  inline  HWINDOW SCAPI SciterCreateWindow ( UINT creationFlags,LPRECT frame, SciterWindowDelegate* delegate, LPVOID delegateParam, HWINDOW parent) { return SAPI()->SciterCreateWindow (creationFlags,frame,delegate,delegateParam,parent); }
#endif

  inline SCDOM_RESULT SCAPI Sciter_UseElement(HELEMENT he) { return SAPI()->Sciter_UseElement(he); }
  inline SCDOM_RESULT SCAPI Sciter_UnuseElement(HELEMENT he) { return SAPI()->Sciter_UnuseElement(he); }
  inline SCDOM_RESULT SCAPI SciterGetRootElement(HWINDOW hwnd, HELEMENT *phe) { return SAPI()->SciterGetRootElement(hwnd, phe); }
  inline SCDOM_RESULT SCAPI SciterGetFocusElement(HWINDOW hwnd, HELEMENT *phe) { return SAPI()->SciterGetFocusElement(hwnd, phe); }
  inline SCDOM_RESULT SCAPI SciterFindElement(HWINDOW hwnd, POINT pt, HELEMENT* phe) { return SAPI()->SciterFindElement(hwnd,pt,phe); }
  inline SCDOM_RESULT SCAPI SciterGetChildrenCount(HELEMENT he, UINT* count) { return SAPI()->SciterGetChildrenCount(he, count); }
  inline SCDOM_RESULT SCAPI SciterGetNthChild(HELEMENT he, UINT n, HELEMENT* phe) { return SAPI()->SciterGetNthChild(he,n,phe); }
  inline SCDOM_RESULT SCAPI SciterGetParentElement(HELEMENT he, HELEMENT* p_parent_he) { return SAPI()->SciterGetParentElement(he,p_parent_he); }
  inline SCDOM_RESULT SCAPI SciterGetElementHtmlCB(HELEMENT he, SBOOL outer, LPCBYTE_RECEIVER* rcv, LPVOID rcv_param) { return SAPI()->SciterGetElementHtmlCB( he, outer, rcv, rcv_param); }
  inline SCDOM_RESULT SCAPI SciterGetElementTextCB(HELEMENT he, LPCWSTR_RECEIVER* rcv, LPVOID rcv_param) { return SAPI()->SciterGetElementTextCB(he, rcv, rcv_param); }
  inline SCDOM_RESULT SCAPI SciterSetElementText(HELEMENT he, LPCWSTR utf16, UINT length) { return SAPI()->SciterSetElementText(he, utf16, length); }
  inline SCDOM_RESULT SCAPI SciterGetAttributeCount(HELEMENT he, LPUINT p_count) { return SAPI()->SciterGetAttributeCount(he, p_count); }
  inline SCDOM_RESULT SCAPI SciterGetNthAttributeNameCB(HELEMENT he, UINT n, LPCSTR_RECEIVER* rcv, LPVOID rcv_param) { return SAPI()->SciterGetNthAttributeNameCB(he,n,rcv,rcv_param); }
  inline SCDOM_RESULT SCAPI SciterGetNthAttributeValueCB(HELEMENT he, UINT n, LPCWSTR_RECEIVER* rcv, LPVOID rcv_param) { return SAPI()->SciterGetNthAttributeValueCB(he, n, rcv, rcv_param); }
  inline SCDOM_RESULT SCAPI SciterGetAttributeByNameCB(HELEMENT he, LPCSTR name, LPCWSTR_RECEIVER* rcv, LPVOID rcv_param) { return SAPI()->SciterGetAttributeByNameCB(he,name,rcv,rcv_param); }
  inline SCDOM_RESULT SCAPI SciterSetAttributeByName(HELEMENT he, LPCSTR name, LPCWSTR value) { return SAPI()->SciterSetAttributeByName(he,name,value); }
  inline SCDOM_RESULT SCAPI SciterClearAttributes(HELEMENT he) { return SAPI()->SciterClearAttributes(he); }
  inline SCDOM_RESULT SCAPI SciterGetElementIndex(HELEMENT he, LPUINT p_index) { return SAPI()->SciterGetElementIndex(he,p_index); }
  inline SCDOM_RESULT SCAPI SciterGetElementType(HELEMENT he, LPCSTR* p_type) { return SAPI()->SciterGetElementType(he,p_type); }
  inline SCDOM_RESULT SCAPI SciterGetElementTypeCB(HELEMENT he, LPCSTR_RECEIVER* rcv, LPVOID rcv_param) { return SAPI()->SciterGetElementTypeCB(he,rcv,rcv_param); }
  inline SCDOM_RESULT SCAPI SciterGetStyleAttributeCB(HELEMENT he, LPCSTR name, LPCWSTR_RECEIVER* rcv, LPVOID rcv_param) { return SAPI()->SciterGetStyleAttributeCB(he,name,rcv,rcv_param); }
  inline SCDOM_RESULT SCAPI SciterSetStyleAttribute(HELEMENT he, LPCSTR name, LPCWSTR value) { return SAPI()->SciterSetStyleAttribute(he,name,value); }
  inline SCDOM_RESULT SCAPI SciterGetElementLocation(HELEMENT he, LPRECT p_location, UINT areas /*ELEMENT_AREAS*/) { return SAPI()->SciterGetElementLocation(he,p_location,areas); }
  inline SCDOM_RESULT SCAPI SciterScrollToView(HELEMENT he, UINT SciterScrollFlags) { return SAPI()->SciterScrollToView(he,SciterScrollFlags); }
  inline SCDOM_RESULT SCAPI SciterUpdateElement(HELEMENT he, SBOOL andForceRender) { return SAPI()->SciterUpdateElement(he,andForceRender); }
  inline SCDOM_RESULT SCAPI SciterRefreshElementArea(HELEMENT he, RECT rc) { return SAPI()->SciterRefreshElementArea(he,rc); }
  inline SCDOM_RESULT SCAPI SciterSetCapture(HELEMENT he) { return SAPI()->SciterSetCapture(he); }
  inline SCDOM_RESULT SCAPI SciterReleaseCapture(HELEMENT he) { return SAPI()->SciterReleaseCapture(he); }
  inline SCDOM_RESULT SCAPI SciterGetElementHwnd(HELEMENT he, HWINDOW* p_hwnd, SBOOL rootWindow) { return SAPI()->SciterGetElementHwnd(he,p_hwnd,rootWindow); }
  inline SCDOM_RESULT SCAPI SciterCombineURL(HELEMENT he, LPWSTR szUrlBuffer, UINT UrlBufferSize) { return SAPI()->SciterCombineURL(he,szUrlBuffer,UrlBufferSize); }
  inline SCDOM_RESULT SCAPI SciterSelectElements(HELEMENT  he, LPCSTR    CSS_selectors, SciterElementCallback* callback, LPVOID param) { return SAPI()->SciterSelectElements(he,CSS_selectors,callback,param); }
  inline SCDOM_RESULT SCAPI SciterSelectElementsW(HELEMENT  he, LPCWSTR   CSS_selectors, SciterElementCallback* callback, LPVOID param) { return SAPI()->SciterSelectElementsW(he,CSS_selectors,callback,param); }
  inline SCDOM_RESULT SCAPI SciterSelectParent(HELEMENT  he, LPCSTR    selector, UINT      depth, HELEMENT* heFound) { return SAPI()->SciterSelectParent(he,selector,depth,heFound); }
  inline SCDOM_RESULT SCAPI SciterSelectParentW(HELEMENT  he, LPCWSTR   selector, UINT      depth, HELEMENT* heFound) { return SAPI()->SciterSelectParentW(he,selector,depth,heFound); }
  inline SCDOM_RESULT SCAPI SciterSetElementHtml(HELEMENT he, const BYTE* html, UINT htmlLength, UINT where) { return SAPI()->SciterSetElementHtml(he,html,htmlLength,where); }
  inline SCDOM_RESULT SCAPI SciterGetElementUID(HELEMENT he, UINT* puid) { return SAPI()->SciterGetElementUID(he,puid); }
  inline SCDOM_RESULT SCAPI SciterGetElementByUID(HWINDOW hwnd, UINT uid, HELEMENT* phe) { return SAPI()->SciterGetElementByUID(hwnd,uid,phe); }
  inline SCDOM_RESULT SCAPI SciterShowPopup(HELEMENT hePopup, HELEMENT heAnchor, UINT placement) { return SAPI()->SciterShowPopup(hePopup,heAnchor,placement); }
  inline SCDOM_RESULT SCAPI SciterShowPopupAt(HELEMENT hePopup, POINT pos, UINT placement) { return SAPI()->SciterShowPopupAt(hePopup,pos,placement); }
  inline SCDOM_RESULT SCAPI SciterHidePopup(HELEMENT he) { return SAPI()->SciterHidePopup(he); }
  inline SCDOM_RESULT SCAPI SciterGetElementState( HELEMENT he, UINT* pstateBits) { return SAPI()->SciterGetElementState(he,pstateBits); }
  inline SCDOM_RESULT SCAPI SciterSetElementState( HELEMENT he, UINT stateBitsToSet, UINT stateBitsToClear, SBOOL updateView) { return SAPI()->SciterSetElementState(he,stateBitsToSet,stateBitsToClear,updateView); }
  inline SCDOM_RESULT SCAPI SciterCreateElement( LPCSTR tagname, LPCWSTR textOrNull, /*out*/ HELEMENT *phe ) { return SAPI()->SciterCreateElement(tagname,textOrNull,phe ); }
  inline SCDOM_RESULT SCAPI SciterCloneElement( HELEMENT he, /*out*/ HELEMENT *phe ) { return SAPI()->SciterCloneElement(he,phe ); }
  inline SCDOM_RESULT SCAPI SciterInsertElement( HELEMENT he, HELEMENT hparent, UINT index ) { return SAPI()->SciterInsertElement(he,hparent,index ); }
  inline SCDOM_RESULT SCAPI SciterDetachElement( HELEMENT he ) { return SAPI()->SciterDetachElement( he ); }
  inline SCDOM_RESULT SCAPI SciterDeleteElement(HELEMENT he) { return SAPI()->SciterDeleteElement(he); }
  inline SCDOM_RESULT SCAPI SciterSetTimer( HELEMENT he, UINT milliseconds, UINT_PTR timer_id ) { return SAPI()->SciterSetTimer(he,milliseconds,timer_id ); }
  inline SCDOM_RESULT SCAPI SciterDetachEventHandler( HELEMENT he, LPELEMENT_EVENT_PROC pep, LPVOID tag ) { return SAPI()->SciterDetachEventHandler(he,pep,tag ); }
  inline SCDOM_RESULT SCAPI SciterAttachEventHandler( HELEMENT he, LPELEMENT_EVENT_PROC pep, LPVOID tag ) { return SAPI()->SciterAttachEventHandler( he,pep,tag ); }
  inline SCDOM_RESULT SCAPI SciterWindowAttachEventHandler( HWINDOW hwndLayout, LPELEMENT_EVENT_PROC pep, LPVOID tag, UINT subscription ) { return SAPI()->SciterWindowAttachEventHandler(hwndLayout,pep,tag,subscription ); }
  inline SCDOM_RESULT SCAPI SciterWindowDetachEventHandler( HWINDOW hwndLayout, LPELEMENT_EVENT_PROC pep, LPVOID tag ) { return SAPI()->SciterWindowDetachEventHandler(hwndLayout,pep,tag ); }
  inline SCDOM_RESULT SCAPI SciterSendEvent( HELEMENT he, UINT appEventCode, HELEMENT heSource, UINT_PTR reason, /*out*/ SBOOL* handled) { return SAPI()->SciterSendEvent( he,appEventCode,heSource,reason,handled); }
  inline SCDOM_RESULT SCAPI SciterPostEvent( HELEMENT he, UINT appEventCode, HELEMENT heSource, UINT_PTR reason) { return SAPI()->SciterPostEvent(he,appEventCode,heSource,reason); }
  inline SCDOM_RESULT SCAPI SciterFireEvent( const BEHAVIOR_EVENT_PARAMS* evt, SBOOL post, SBOOL *handled) { return SAPI()->SciterFireEvent( evt, post, handled ); }
  inline SCDOM_RESULT SCAPI SciterCallBehaviorMethod(HELEMENT he, struct METHOD_PARAMS* params) { return SAPI()->SciterCallBehaviorMethod(he,params); }
  inline SCDOM_RESULT SCAPI SciterRequestElementData( HELEMENT he, LPCWSTR url, UINT dataType, HELEMENT initiator ) { return SAPI()->SciterRequestElementData(he,url,dataType,initiator ); }
  inline SCDOM_RESULT SCAPI SciterHttpRequest( HELEMENT he, LPCWSTR url, UINT dataType, UINT requestType, struct REQUEST_PARAM* requestParams, UINT nParams) { return SAPI()->SciterHttpRequest(he,url,dataType,requestType,requestParams,nParams); }
  inline SCDOM_RESULT SCAPI SciterGetScrollInfo( HELEMENT he, LPPOINT scrollPos, LPRECT viewRect, LPSIZE contentSize ) { return SAPI()->SciterGetScrollInfo( he,scrollPos,viewRect,contentSize ); }
  inline SCDOM_RESULT SCAPI SciterSetScrollPos( HELEMENT he, POINT scrollPos, SBOOL smooth ) { return SAPI()->SciterSetScrollPos( he,scrollPos,smooth ); }
  inline SCDOM_RESULT SCAPI SciterGetElementIntrinsicWidths( HELEMENT he, INT* pMinWidth, INT* pMaxWidth ) { return SAPI()->SciterGetElementIntrinsicWidths(he,pMinWidth,pMaxWidth ); }
  inline SCDOM_RESULT SCAPI SciterGetElementIntrinsicHeight( HELEMENT he, INT forWidth, INT* pHeight ) { return SAPI()->SciterGetElementIntrinsicHeight( he,forWidth,pHeight ); }
  inline SCDOM_RESULT SCAPI SciterIsElementVisible( HELEMENT he, SBOOL* pVisible) { return SAPI()->SciterIsElementVisible( he,pVisible); }
  inline SCDOM_RESULT SCAPI SciterIsElementEnabled( HELEMENT he, SBOOL* pEnabled ) { return SAPI()->SciterIsElementEnabled( he, pEnabled ); }
  inline SCDOM_RESULT SCAPI SciterSortElements( HELEMENT he, UINT firstIndex, UINT lastIndex, ELEMENT_COMPARATOR* cmpFunc, LPVOID cmpFuncParam ) { return SAPI()->SciterSortElements( he, firstIndex, lastIndex, cmpFunc, cmpFuncParam ); }
  inline SCDOM_RESULT SCAPI SciterSwapElements( HELEMENT he1, HELEMENT he2 ) { return SAPI()->SciterSwapElements( he1,he2 ); }
  inline SCDOM_RESULT SCAPI SciterTraverseUIEvent( UINT evt, LPVOID eventCtlStruct, SBOOL* bOutProcessed ) { return SAPI()->SciterTraverseUIEvent( evt,eventCtlStruct,bOutProcessed ); }
  inline SCDOM_RESULT SCAPI SciterCallScriptingMethod( HELEMENT he, LPCSTR name, const VALUE* argv, UINT argc, VALUE* retval ) { return SAPI()->SciterCallScriptingMethod( he,name,argv,argc,retval ); }
  inline SCDOM_RESULT SCAPI SciterCallScriptingFunction( HELEMENT he, LPCSTR name, const VALUE* argv, UINT argc, VALUE* retval ) { return SAPI()->SciterCallScriptingFunction( he,name,argv,argc,retval ); }
  inline SCDOM_RESULT SCAPI SciterEvalElementScript( HELEMENT he, LPCWSTR script, UINT scriptLength, VALUE* retval ) { return SAPI()->SciterEvalElementScript( he, script, scriptLength, retval ); }
  inline SCDOM_RESULT SCAPI SciterAttachHwndToElement(HELEMENT he, HWINDOW hwnd) { return SAPI()->SciterAttachHwndToElement(he,hwnd); }
  inline SCDOM_RESULT SCAPI SciterControlGetType( HELEMENT he, /*CTL_TYPE*/ UINT *pType ) { return SAPI()->SciterControlGetType( he, pType ); }
  inline SCDOM_RESULT SCAPI SciterGetValue( HELEMENT he, VALUE* pval ) { return SAPI()->SciterGetValue( he,pval ); }
  inline SCDOM_RESULT SCAPI SciterSetValue( HELEMENT he, const VALUE* pval ) { return SAPI()->SciterSetValue( he, pval ); }
  inline SCDOM_RESULT SCAPI SciterGetExpando( HELEMENT he, VALUE* pval, SBOOL forceCreation ) { return SAPI()->SciterGetExpando( he, pval, forceCreation ); }
  //inline SCDOM_RESULT SCAPI SciterGetObject( HELEMENT he, void* pval, SBOOL forceCreation ) { return SAPI()->SciterGetObject( he, pval, forceCreation ); }
  //inline SCDOM_RESULT SCAPI SciterGetElementNamespace(  HELEMENT he, void* pval) { return SAPI()->SciterGetElementNamespace( he,pval); }
  inline SCDOM_RESULT SCAPI SciterGetHighlightedElement(HWINDOW hwnd, HELEMENT* phe) { return SAPI()->SciterGetHighlightedElement(hwnd, phe); }
  inline SCDOM_RESULT SCAPI SciterSetHighlightedElement(HWINDOW hwnd, HELEMENT he) { return SAPI()->SciterSetHighlightedElement(hwnd,he); }
  inline SCDOM_RESULT SCAPI SciterNodeAddRef(HNODE hn) { return SAPI()->SciterNodeAddRef(hn); }
  inline SCDOM_RESULT SCAPI SciterNodeRelease(HNODE hn) { return SAPI()->SciterNodeRelease(hn); }
  inline SCDOM_RESULT SCAPI SciterNodeCastFromElement(HELEMENT he, HNODE* phn) { return SAPI()->SciterNodeCastFromElement(he,phn); }
  inline SCDOM_RESULT SCAPI SciterNodeCastToElement(HNODE hn, HELEMENT* he) { return SAPI()->SciterNodeCastToElement(hn,he); }
  inline SCDOM_RESULT SCAPI SciterNodeFirstChild(HNODE hn, HNODE* phn) { return SAPI()->SciterNodeFirstChild(hn,phn); }
  inline SCDOM_RESULT SCAPI SciterNodeLastChild(HNODE hn, HNODE* phn) { return SAPI()->SciterNodeLastChild(hn, phn); }
  inline SCDOM_RESULT SCAPI SciterNodeNextSibling(HNODE hn, HNODE* phn) { return SAPI()->SciterNodeNextSibling(hn, phn); }
  inline SCDOM_RESULT SCAPI SciterNodePrevSibling(HNODE hn, HNODE* phn) { return SAPI()->SciterNodePrevSibling(hn,phn); }
  inline SCDOM_RESULT SCAPI SciterNodeParent(HNODE hnode, HELEMENT* pheParent) { return SAPI()->SciterNodeParent(hnode,pheParent) ; }
  inline SCDOM_RESULT SCAPI SciterNodeNthChild(HNODE hnode, UINT n, HNODE* phn) { return SAPI()->SciterNodeNthChild(hnode,n,phn); }
  inline SCDOM_RESULT SCAPI SciterNodeChildrenCount(HNODE hnode, UINT* pn) { return SAPI()->SciterNodeChildrenCount(hnode, pn); }
  inline SCDOM_RESULT SCAPI SciterNodeType(HNODE hnode, UINT* pNodeType /*NODE_TYPE*/) { return SAPI()->SciterNodeType(hnode,pNodeType); }
  inline SCDOM_RESULT SCAPI SciterNodeGetText(HNODE hnode, LPCWSTR_RECEIVER* rcv, LPVOID rcv_param) { return SAPI()->SciterNodeGetText(hnode,rcv,rcv_param); }
  inline SCDOM_RESULT SCAPI SciterNodeSetText(HNODE hnode, LPCWSTR text, UINT textLength) { return SAPI()->SciterNodeSetText(hnode,text,textLength); }
  inline SCDOM_RESULT SCAPI SciterNodeInsert(HNODE hnode, UINT where /*NODE_INS_TARGET*/, HNODE what) { return SAPI()->SciterNodeInsert(hnode,where,what); }
  inline SCDOM_RESULT SCAPI SciterNodeRemove(HNODE hnode, SBOOL finalize) { return SAPI()->SciterNodeRemove(hnode,finalize); }
  inline SCDOM_RESULT SCAPI SciterCreateTextNode(LPCWSTR text, UINT textLength, HNODE* phnode) { return SAPI()->SciterCreateTextNode(text,textLength,phnode); }
  inline SCDOM_RESULT SCAPI SciterCreateCommentNode(LPCWSTR text, UINT textLength, HNODE* phnode) { return SAPI()->SciterCreateCommentNode(text,textLength,phnode); }

  //inline HVM   SCAPI SciterGetVM( HWINDOW hwnd )  { return SAPI()->SciterGetVM(hwnd); }

  inline UINT SCAPI ValueInit ( VALUE* pval ) { return SAPI()->ValueInit(pval); }
  inline UINT SCAPI ValueClear ( VALUE* pval ) { return SAPI()->ValueClear(pval); }
  inline UINT SCAPI ValueCompare ( const VALUE* pval1, const VALUE* pval2 ) { return SAPI()->ValueCompare(pval1,pval2); }
  inline UINT SCAPI ValueCopy ( VALUE* pdst, const VALUE* psrc ) { return SAPI()->ValueCopy(pdst, psrc); }
  inline UINT SCAPI ValueIsolate ( VALUE* pdst ) { return SAPI()->ValueIsolate(pdst); }
  inline UINT SCAPI ValueType ( const VALUE* pval, UINT* pType, UINT* pUnits ) { return SAPI()->ValueType(pval,pType,pUnits); }
  inline UINT SCAPI ValueStringData ( const VALUE* pval, LPCWSTR* pChars, UINT* pNumChars ) { return SAPI()->ValueStringData(pval,pChars,pNumChars); }
  inline UINT SCAPI ValueStringDataSet ( VALUE* pval, LPCWSTR chars, UINT numChars, UINT units ) { return SAPI()->ValueStringDataSet(pval, chars, numChars, units); }
  inline UINT SCAPI ValueIntData ( const VALUE* pval, INT* pData ) { return SAPI()->ValueIntData ( pval, pData ); }
  inline UINT SCAPI ValueIntDataSet ( VALUE* pval, INT data, UINT type, UINT units ) { return SAPI()->ValueIntDataSet ( pval, data,type,units ); }
  inline UINT SCAPI ValueInt64Data ( const VALUE* pval, INT64* pData ) { return SAPI()->ValueInt64Data ( pval,pData ); }
  inline UINT SCAPI ValueInt64DataSet ( VALUE* pval, INT64 data, UINT type, UINT units ) { return SAPI()->ValueInt64DataSet ( pval,data,type,units ); }
  inline UINT SCAPI ValueFloatData ( const VALUE* pval, FLOAT_VALUE* pData ) { return SAPI()->ValueFloatData ( pval,pData ); }
  inline UINT SCAPI ValueFloatDataSet ( VALUE* pval, FLOAT_VALUE data, UINT type, UINT units ) { return SAPI()->ValueFloatDataSet ( pval,data,type,units ); }
  inline UINT SCAPI ValueBinaryData ( const VALUE* pval, LPCBYTE* pBytes, UINT* pnBytes ) { return SAPI()->ValueBinaryData ( pval,pBytes,pnBytes ); }
  inline UINT SCAPI ValueBinaryDataSet ( VALUE* pval, LPCBYTE pBytes, UINT nBytes, UINT type, UINT units ) { return SAPI()->ValueBinaryDataSet ( pval,pBytes,nBytes,type,units ); }
  inline UINT SCAPI ValueElementsCount ( const VALUE* pval, INT* pn) { return SAPI()->ValueElementsCount ( pval,pn); }
  inline UINT SCAPI ValueNthElementValue ( const VALUE* pval, INT n, VALUE* pretval) { return SAPI()->ValueNthElementValue ( pval, n, pretval); }
  inline UINT SCAPI ValueNthElementValueSet ( VALUE* pval, INT n, const VALUE* pval_to_set) { return SAPI()->ValueNthElementValueSet ( pval,n,pval_to_set); }
  inline UINT SCAPI ValueNthElementKey ( const VALUE* pval, INT n, VALUE* pretval) { return SAPI()->ValueNthElementKey ( pval,n,pretval); }
  inline UINT SCAPI ValueEnumElements ( const VALUE* pval, KeyValueCallback* penum, LPVOID param) { return SAPI()->ValueEnumElements (pval,penum,param); }
  inline UINT SCAPI ValueSetValueToKey ( VALUE* pval, const VALUE* pkey, const VALUE* pval_to_set) { return SAPI()->ValueSetValueToKey ( pval, pkey, pval_to_set); }
  inline UINT SCAPI ValueGetValueOfKey ( const VALUE* pval, const VALUE* pkey, VALUE* pretval) { return SAPI()->ValueGetValueOfKey ( pval, pkey,pretval); }
  inline UINT SCAPI ValueToString ( VALUE* pval, UINT how ) { return SAPI()->ValueToString ( pval,how ); }
  inline UINT SCAPI ValueFromString ( VALUE* pval, LPCWSTR str, UINT strLength, UINT how ) { return SAPI()->ValueFromString ( pval, str,strLength,how ); }
  inline UINT SCAPI ValueInvoke ( const VALUE* pval, VALUE* pthis, UINT argc, const VALUE* argv, VALUE* pretval, LPCWSTR url) { return SAPI()->ValueInvoke ( pval, pthis, argc, argv, pretval, url); }
  inline UINT SCAPI ValueNativeFunctorSet (VALUE* pval, NATIVE_FUNCTOR_INVOKE*  pinvoke, NATIVE_FUNCTOR_RELEASE* prelease, VOID* tag ) { return SAPI()->ValueNativeFunctorSet ( pval, pinvoke,prelease,tag); }
  inline SBOOL SCAPI ValueIsNativeFunctor ( const VALUE* pval) { return SAPI()->ValueIsNativeFunctor (pval); }

#if defined(WINDOWS) && !defined(WINDOWLESS)
  inline SBOOL SCAPI SciterCreateOnDirectXWindow(HWINDOW hwnd, IUnknown* pSwapChain) { return SAPI()->SciterCreateOnDirectXWindow(hwnd,pSwapChain); }
  inline SBOOL SCAPI SciterRenderOnDirectXWindow(HWINDOW hwnd, HELEMENT elementToRenderOrNull, SBOOL frontLayer) { return SAPI()->SciterRenderOnDirectXWindow(hwnd,elementToRenderOrNull,frontLayer); }
  inline SBOOL SCAPI SciterRenderOnDirectXTexture(HWINDOW hwnd, HELEMENT elementToRenderOrNull, IUnknown* surface) { return SAPI()->SciterRenderOnDirectXTexture(hwnd,elementToRenderOrNull,surface); }
#endif

  inline   SBOOL SCAPI SciterProcX(HWINDOW hwnd, SCITER_X_MSG* pMsg) { return SAPI()->SciterProcX(hwnd, pMsg); }
#ifdef __cplusplus
  template<class MSG>
  inline   SBOOL SCAPI SciterProcX(HWINDOW hwnd, const MSG &msg) {
     static_assert(offsetof(MSG, header) == 0, "must contain SCITER_X_MSG as first memeber");
     static_assert(std::is_same<decltype(MSG::header), SCITER_X_MSG>::value, "must contain SCITER_X_MSG");
     return SAPI()->SciterProcX(hwnd, (SCITER_X_MSG*)(&msg));
  }
#endif

  inline UINT64 SCAPI SciterAtomValue(const char* name) { return SAPI()->SciterAtomValue(name); }
  inline UINT64 SCAPI SciterAtomNameCB(UINT64 atomv, LPCSTR_RECEIVER* rcv, LPVOID rcv_param) { return SAPI()->SciterAtomNameCB(atomv, rcv, rcv_param); }
  inline SBOOL   SCAPI SciterSetGlobalAsset(som_asset_t* pass) { return SAPI()->SciterSetGlobalAsset(pass); }

  inline UINT   SCAPI SciterElementUnwrap(const VALUE* pval, HELEMENT* ppElement) { return SAPI()->SciterElementUnwrap(pval, ppElement); }
  inline UINT   SCAPI SciterElementWrap(VALUE* pval, HELEMENT pElement) { return SAPI()->SciterElementWrap(pval, pElement); }

  inline UINT   SCAPI SciterNodeUnwrap(const VALUE* pval, HNODE* ppNode) { return SAPI()->SciterNodeUnwrap(pval, ppNode); }
  inline UINT   SCAPI SciterNodeWrap(VALUE* pval, HNODE pNode) { return SAPI()->SciterNodeWrap(pval, pNode); }

 HSARCHIVE SCAPI SciterOpenArchive (LPCBYTE archiveData, UINT archiveDataLength) { return SAPI()->SciterOpenArchive (archiveData,archiveDataLength); }
 SBOOL SCAPI SciterGetArchiveItem (HSARCHIVE harc, LPCWSTR path, LPCBYTE* pdata, UINT* pdataLength){ return SAPI()->SciterGetArchiveItem (harc,path,pdata,pdataLength); }
 SBOOL SCAPI SciterCloseArchive (HSARCHIVE harc) { return SAPI()->SciterCloseArchive(harc); }