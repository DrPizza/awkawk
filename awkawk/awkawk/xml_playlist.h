// this is an edited version of the Platform SDK's playlist.h, because some GUIDs are wrong in that file.

/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef XML_PLAYLIST__H
#define XML_PLAYLIST__H

/* Forward Declarations */ 

#ifndef __IXMLPlayListItem_FWD_DEFINED__
#define __IXMLPlayListItem_FWD_DEFINED__
typedef interface IXMLPlayListItem IXMLPlayListItem;
#endif 	/* __IXMLPlayListItem_FWD_DEFINED__ */


#ifndef __IXMLPlayList_FWD_DEFINED__
#define __IXMLPlayList_FWD_DEFINED__
typedef interface IXMLPlayList IXMLPlayList;
#endif 	/* __IXMLPlayList_FWD_DEFINED__ */

/* header files for imported files */
#include "unknwn.h"
#include "strmif.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_playlist_0000 */
/* [local] */ 


enum XMLPlayListItemFlags
    {	XMLPLAYLISTITEM_CANSKIP	= 0x1,
	XMLPLAYLISTITEM_CANBIND	= 0x2
    };


extern RPC_IF_HANDLE __MIDL_itf_playlist_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_playlist_0000_v0_0_s_ifspec;

#ifndef __IXMLPlayListItem_INTERFACE_DEFINED__
#define __IXMLPlayListItem_INTERFACE_DEFINED__

/* interface IXMLPlayListItem */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IXMLPlayListItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("56a868ff-0ad4-11ce-b0a3-0020af0ba770")
    IXMLPlayListItem : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlags( 
            /* [out] */ DWORD __RPC_FAR *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceCount( 
            /* [out] */ DWORD __RPC_FAR *pdwSources) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceURL( 
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ BSTR __RPC_FAR *pbstrURL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceStart( 
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ REFERENCE_TIME __RPC_FAR *prtStart) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceDuration( 
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ REFERENCE_TIME __RPC_FAR *prtDuration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceStartMarker( 
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ DWORD __RPC_FAR *pdwMarker) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceEndMarker( 
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ DWORD __RPC_FAR *pdwMarker) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceStartMarkerName( 
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ BSTR __RPC_FAR *pbstrStartMarker) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceEndMarkerName( 
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ BSTR __RPC_FAR *pbstrEndMarker) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLinkURL( 
            /* [out] */ BSTR __RPC_FAR *pbstrURL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScanDuration( 
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ REFERENCE_TIME __RPC_FAR *prtScanDuration) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXMLPlayListItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IXMLPlayListItem __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IXMLPlayListItem __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFlags )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceCount )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSources);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceURL )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ BSTR __RPC_FAR *pbstrURL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceStart )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ REFERENCE_TIME __RPC_FAR *prtStart);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceDuration )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ REFERENCE_TIME __RPC_FAR *prtDuration);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceStartMarker )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ DWORD __RPC_FAR *pdwMarker);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceEndMarker )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ DWORD __RPC_FAR *pdwMarker);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceStartMarkerName )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ BSTR __RPC_FAR *pbstrStartMarker);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceEndMarkerName )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ BSTR __RPC_FAR *pbstrEndMarker);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLinkURL )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrURL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScanDuration )( 
            IXMLPlayListItem __RPC_FAR * This,
            /* [in] */ DWORD dwSourceIndex,
            /* [out] */ REFERENCE_TIME __RPC_FAR *prtScanDuration);
        
        END_INTERFACE
    } IXMLPlayListItemVtbl;

    interface IXMLPlayListItem
    {
        CONST_VTBL struct IXMLPlayListItemVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXMLPlayListItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXMLPlayListItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXMLPlayListItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IXMLPlayListItem_GetFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetFlags(This,pdwFlags)

#define IXMLPlayListItem_GetSourceCount(This,pdwSources)	\
    (This)->lpVtbl -> GetSourceCount(This,pdwSources)

#define IXMLPlayListItem_GetSourceURL(This,dwSourceIndex,pbstrURL)	\
    (This)->lpVtbl -> GetSourceURL(This,dwSourceIndex,pbstrURL)

#define IXMLPlayListItem_GetSourceStart(This,dwSourceIndex,prtStart)	\
    (This)->lpVtbl -> GetSourceStart(This,dwSourceIndex,prtStart)

#define IXMLPlayListItem_GetSourceDuration(This,dwSourceIndex,prtDuration)	\
    (This)->lpVtbl -> GetSourceDuration(This,dwSourceIndex,prtDuration)

#define IXMLPlayListItem_GetSourceStartMarker(This,dwSourceIndex,pdwMarker)	\
    (This)->lpVtbl -> GetSourceStartMarker(This,dwSourceIndex,pdwMarker)

#define IXMLPlayListItem_GetSourceEndMarker(This,dwSourceIndex,pdwMarker)	\
    (This)->lpVtbl -> GetSourceEndMarker(This,dwSourceIndex,pdwMarker)

#define IXMLPlayListItem_GetSourceStartMarkerName(This,dwSourceIndex,pbstrStartMarker)	\
    (This)->lpVtbl -> GetSourceStartMarkerName(This,dwSourceIndex,pbstrStartMarker)

#define IXMLPlayListItem_GetSourceEndMarkerName(This,dwSourceIndex,pbstrEndMarker)	\
    (This)->lpVtbl -> GetSourceEndMarkerName(This,dwSourceIndex,pbstrEndMarker)

#define IXMLPlayListItem_GetLinkURL(This,pbstrURL)	\
    (This)->lpVtbl -> GetLinkURL(This,pbstrURL)

#define IXMLPlayListItem_GetScanDuration(This,dwSourceIndex,prtScanDuration)	\
    (This)->lpVtbl -> GetScanDuration(This,dwSourceIndex,prtScanDuration)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetFlags_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB IXMLPlayListItem_GetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetSourceCount_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwSources);


void __RPC_STUB IXMLPlayListItem_GetSourceCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetSourceURL_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [in] */ DWORD dwSourceIndex,
    /* [out] */ BSTR __RPC_FAR *pbstrURL);


void __RPC_STUB IXMLPlayListItem_GetSourceURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetSourceStart_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [in] */ DWORD dwSourceIndex,
    /* [out] */ REFERENCE_TIME __RPC_FAR *prtStart);


void __RPC_STUB IXMLPlayListItem_GetSourceStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetSourceDuration_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [in] */ DWORD dwSourceIndex,
    /* [out] */ REFERENCE_TIME __RPC_FAR *prtDuration);


void __RPC_STUB IXMLPlayListItem_GetSourceDuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetSourceStartMarker_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [in] */ DWORD dwSourceIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMarker);


void __RPC_STUB IXMLPlayListItem_GetSourceStartMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetSourceEndMarker_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [in] */ DWORD dwSourceIndex,
    /* [out] */ DWORD __RPC_FAR *pdwMarker);


void __RPC_STUB IXMLPlayListItem_GetSourceEndMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetSourceStartMarkerName_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [in] */ DWORD dwSourceIndex,
    /* [out] */ BSTR __RPC_FAR *pbstrStartMarker);


void __RPC_STUB IXMLPlayListItem_GetSourceStartMarkerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetSourceEndMarkerName_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [in] */ DWORD dwSourceIndex,
    /* [out] */ BSTR __RPC_FAR *pbstrEndMarker);


void __RPC_STUB IXMLPlayListItem_GetSourceEndMarkerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetLinkURL_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrURL);


void __RPC_STUB IXMLPlayListItem_GetLinkURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayListItem_GetScanDuration_Proxy( 
    IXMLPlayListItem __RPC_FAR * This,
    /* [in] */ DWORD dwSourceIndex,
    /* [out] */ REFERENCE_TIME __RPC_FAR *prtScanDuration);


void __RPC_STUB IXMLPlayListItem_GetScanDuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IXMLPlayListItem_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_playlist_0348 */
/* [local] */ 


enum XMLPlayListFlags
    {	XMLPLAYLIST_STARTINSCANMODE	= 0x1,
	XMLPLAYLIST_FORCEBANNER	= 0x2
    };

enum XMLPlayListEventFlags
    {	XMLPLAYLISTEVENT_RESUME	= 0,
	XMLPLAYLISTEVENT_BREAK	= 0x1,
	XMLPLAYLISTEVENT_NEXT	= 0x2,
	XMLPLAYLISTEVENT_MASK	= 0xf,
	XMLPLAYLISTEVENT_REFRESH	= 0x10
    };


extern RPC_IF_HANDLE __MIDL_itf_playlist_0348_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_playlist_0348_v0_0_s_ifspec;

#ifndef __IXMLPlayList_INTERFACE_DEFINED__
#define __IXMLPlayList_INTERFACE_DEFINED__

/* interface IXMLPlayList */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IXMLPlayList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("56a868fe-0ad4-11ce-b0a3-0020af0ba770")
    IXMLPlayList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlags( 
            /* [out] */ DWORD __RPC_FAR *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemCount( 
            /* [out] */ DWORD __RPC_FAR *pdwItems) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItem( 
            /* [in] */ DWORD dwItemIndex,
            /* [out] */ IXMLPlayListItem __RPC_FAR *__RPC_FAR *ppItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNamedEvent( 
            /* [in] */ WCHAR __RPC_FAR *pwszEventName,
            /* [in] */ DWORD dwItemIndex,
            /* [out] */ IXMLPlayListItem __RPC_FAR *__RPC_FAR *ppItem,
            /* [out] */ DWORD __RPC_FAR *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRepeatInfo( 
            /* [out] */ DWORD __RPC_FAR *pdwRepeatCount,
            /* [out] */ DWORD __RPC_FAR *pdwRepeatStart,
            /* [out] */ DWORD __RPC_FAR *pdwRepeatEnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXMLPlayListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IXMLPlayList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IXMLPlayList __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IXMLPlayList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFlags )( 
            IXMLPlayList __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItemCount )( 
            IXMLPlayList __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwItems);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetItem )( 
            IXMLPlayList __RPC_FAR * This,
            /* [in] */ DWORD dwItemIndex,
            /* [out] */ IXMLPlayListItem __RPC_FAR *__RPC_FAR *ppItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNamedEvent )( 
            IXMLPlayList __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwszEventName,
            /* [in] */ DWORD dwItemIndex,
            /* [out] */ IXMLPlayListItem __RPC_FAR *__RPC_FAR *ppItem,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRepeatInfo )( 
            IXMLPlayList __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwRepeatCount,
            /* [out] */ DWORD __RPC_FAR *pdwRepeatStart,
            /* [out] */ DWORD __RPC_FAR *pdwRepeatEnd);
        
        END_INTERFACE
    } IXMLPlayListVtbl;

    interface IXMLPlayList
    {
        CONST_VTBL struct IXMLPlayListVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXMLPlayList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXMLPlayList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXMLPlayList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IXMLPlayList_GetFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetFlags(This,pdwFlags)

#define IXMLPlayList_GetItemCount(This,pdwItems)	\
    (This)->lpVtbl -> GetItemCount(This,pdwItems)

#define IXMLPlayList_GetItem(This,dwItemIndex,ppItem)	\
    (This)->lpVtbl -> GetItem(This,dwItemIndex,ppItem)

#define IXMLPlayList_GetNamedEvent(This,pwszEventName,dwItemIndex,ppItem,pdwFlags)	\
    (This)->lpVtbl -> GetNamedEvent(This,pwszEventName,dwItemIndex,ppItem,pdwFlags)

#define IXMLPlayList_GetRepeatInfo(This,pdwRepeatCount,pdwRepeatStart,pdwRepeatEnd)	\
    (This)->lpVtbl -> GetRepeatInfo(This,pdwRepeatCount,pdwRepeatStart,pdwRepeatEnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IXMLPlayList_GetFlags_Proxy( 
    IXMLPlayList __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB IXMLPlayList_GetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayList_GetItemCount_Proxy( 
    IXMLPlayList __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwItems);


void __RPC_STUB IXMLPlayList_GetItemCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayList_GetItem_Proxy( 
    IXMLPlayList __RPC_FAR * This,
    /* [in] */ DWORD dwItemIndex,
    /* [out] */ IXMLPlayListItem __RPC_FAR *__RPC_FAR *ppItem);


void __RPC_STUB IXMLPlayList_GetItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayList_GetNamedEvent_Proxy( 
    IXMLPlayList __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwszEventName,
    /* [in] */ DWORD dwItemIndex,
    /* [out] */ IXMLPlayListItem __RPC_FAR *__RPC_FAR *ppItem,
    /* [out] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB IXMLPlayList_GetNamedEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IXMLPlayList_GetRepeatInfo_Proxy( 
    IXMLPlayList __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwRepeatCount,
    /* [out] */ DWORD __RPC_FAR *pdwRepeatStart,
    /* [out] */ DWORD __RPC_FAR *pdwRepeatEnd);


void __RPC_STUB IXMLPlayList_GetRepeatInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IXMLPlayList_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_playlist_0351 */
/* [local] */ 

EXTERN_GUID(IID_IXMLPlayListItem,0x56a868ff,0x0ad4,0x11ce,0xb0,0xa3,0x0,0x20,0xaf,0x0b,0xa7,0x70);
EXTERN_GUID(IID_IXMLPlayList,0x56a868fe,0x0ad4,0x11ce,0xb0,0xa3,0x0,0x20,0xaf,0x0b,0xa7,0x70);


extern RPC_IF_HANDLE __MIDL_itf_playlist_0351_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_playlist_0351_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
