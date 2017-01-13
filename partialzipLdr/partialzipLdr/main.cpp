#include <windows.h>
#include <comutil.h>
#include "partial/partial.h"

#pragma comment(lib, "comsuppw.lib")

struct IPartialZipDownloader: public IUnknown{
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
	virtual BOOL WINAPI DownloadFileFromZip(char* szFileName, void* userInfo, PartialZipGetFileCallback fn) = 0;
	virtual int WINAPI GetFileSize(char* szFileName) = 0;
};

class CZipFile:public IPartialZipDownloader {
private:
	ZipInfo* zip;
	int m_nRefCount;
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		ppvObject = (void**)this;
		return 0;
	}
	ULONG STDMETHODCALLTYPE AddRef(void) { return InterlockedIncrement((LONG*)&m_nRefCount); };
	ULONG STDMETHODCALLTYPE Release(void) {
		int result = InterlockedDecrement((LONG*)&m_nRefCount);
		if (!result)
			delete this;
		return result;
	}
	BOOL WINAPI DownloadFileFromZip(char* szFileName, void* userInfo, PartialZipGetFileCallback fn);
	int WINAPI GetFileSize(char* szFileName);
	CZipFile(char* szUrl):m_nRefCount(1) {
		zip = PartialZipInit(szUrl);
	}
	~CZipFile() {
		if (zip)
			PartialZipRelease(zip);
	}
};

BOOL WINAPI CZipFile::DownloadFileFromZip(char* szFileName, void* userInfo, PartialZipGetFileCallback fn) {
	if (!this->zip) return false;
	CDFile* file = PartialZipFindFile(this->zip, szFileName);
	if (file) {
		return PartialZipGetFile(this->zip, file, fn, userInfo);
	}
	return false;
}

int WINAPI CZipFile::GetFileSize(char* szFileName) {
	if (!this->zip)	return 0;
	CDFile* file = PartialZipFindFile(this->zip, szFileName);
	if (file) return file->size;
	return 0;
}

EXTERN_C void WINAPI NewZipDownloader(IPartialZipDownloader** downloader, BSTR szZipUrl) {
	_bstr_t url = szZipUrl;
	*downloader = new CZipFile((char*)url);
}