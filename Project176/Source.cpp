#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winnetwk.h>

#pragma comment(lib, "mpr.lib")
#pragma comment(lib, "IPHLPAPI.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

BOOL WINAPI enumerateResources();
void PrintError(FILE *errlog, CONST CHAR *szMsg);
void PrintErrorCode(FILE *errlog, CONST CHAR *szMsg, DWORD dwErrcode);
void ShowResource(DWORD dwNesting, LPNETRESOURCE lpnrLocal);
BOOL WINAPI _enumerateResources(LPNETRESOURCE lpnr, FILE *errlog, DWORD dwNesting);

BOOL WINAPI enumerateResources()
{
	LPNETRESOURCE lpnr = NULL;

	return _enumerateResources(lpnr, NULL, 0);
}

BOOL WINAPI _enumerateResources(LPNETRESOURCE lpnr, FILE *errlog, DWORD dwNesting)
{
	DWORD dwResult, dwResultEnum;
	HANDLE hEnum;
	DWORD cbBuffer = 16384;
	DWORD cEntries = -1; // Search for all objects
	LPNETRESOURCE lpnrLocal;
	DWORD i;

	// Вызов функции WNetOpenEnum для начала перечисления компьютеров. 
	dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, 0, lpnr, &hEnum);
	// RESOURCES_GLOBALNET - все сетевые ресурсы
	// RESOURCETYPE_ANY - все типы ресурсов
	// 0 - перечислить все ресурсы
	// lpnrLocal = NULL при первом вызове функции
	// hEnum - дескриптор ресурса

	if (dwResult != NO_ERROR) {
		// Обработка ошибок
		PrintErrorCode(errlog, "WNetOpenEnum failed with error %d\n", dwResult);
		return FALSE;
	}

	// Вызвов функции GlobalAlloc для выделения ресурсов.
	lpnrLocal = (LPNETRESOURCE)GlobalAlloc(GPTR, cbBuffer);
	if (lpnrLocal == NULL) {
		PrintErrorCode(errlog, "WNetOpenEnum failed with error %d\n", dwResult);
		return FALSE;
	}

	do {
		// Инициализируем буфер.
		ZeroMemory(lpnrLocal, cbBuffer);

		// Вызов функции WNetEnumResource для продолжения перечисления
		// доступных ресурсов сети.
		dwResultEnum = WNetEnumResource(hEnum, &cEntries, lpnrLocal, &cbBuffer);
		
		// Если вызов был успешен, то структу ры обрабатываются циклом.
		if (dwResultEnum == NO_ERROR) {
			for (i = 0; i < cEntries; i++) {
				// Функция для отображения содержимого структур NetResources
				ShowResource(dwNesting, &lpnrLocal[i]);
				if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage & RESOURCEUSAGE_CONTAINER)) {
					if (!_enumerateResources(&lpnrLocal[i], errlog, dwNesting + 1)) {
						PrintError(errlog, "enumerateResources function returned FALSE\n");
					}
				}
			}
		}
		// Обработка ошибок
		else if (dwResultEnum != ERROR_NO_MORE_ITEMS) {
			PrintErrorCode(errlog, "WNetEnumResource failed with error %d\n", dwResultEnum);
			break;
		}
	} while (dwResultEnum != ERROR_NO_MORE_ITEMS);

	// Вызов функции GlobalFree для очистки ресурсов
	GlobalFree((HGLOBAL)lpnrLocal);

	// Вызов WNetCloseEnum для остановки перечисления
	dwResult = WNetCloseEnum(hEnum);

	if (dwResult != NO_ERROR) {
		PrintErrorCode(errlog, "WNetCloseEnum failed with error %d\n", dwResult);
		return FALSE;
	}

	return TRUE;
}

void PrintError(FILE *errlog, CONST CHAR *szMsg)
{
	if (errlog) {
		fprintf(errlog, szMsg);
	}
}

void PrintErrorCode(FILE *errlog, CONST CHAR *szMsg, DWORD dwErrcode)
{
	if (errlog) {
		fprintf(errlog, szMsg, dwErrcode);
	}
}

void ShowResource(DWORD dwNesting, LPNETRESOURCE lpnrLocal)
{
	DWORD i;

	printf("-");
	
	for (i = 0; i < dwNesting; i++) {
		printf("  ");
	}
	printf("> ");

	printf("%S ", lpnrLocal->lpRemoteName);
	if (lpnrLocal->lpComment) {
		printf("(%S) ", lpnrLocal->lpComment);
	}
	
	printf("\n   type: ");
	switch (lpnrLocal->dwType) {
	case (RESOURCETYPE_ANY):
		printf("any");
		break;
	case (RESOURCETYPE_DISK):
		printf("disk");
		break;
	case (RESOURCETYPE_PRINT):
		printf("print");
		break;
	default:
		printf("unknown %X", lpnrLocal->dwDisplayType);
		break;
	}
	printf(", display type: ");
	switch (lpnrLocal->dwDisplayType) {
	case (RESOURCEDISPLAYTYPE_GENERIC):
		printf("generic");
		break;
	case (RESOURCEDISPLAYTYPE_DOMAIN):
		printf("domain");
		break;
	case (RESOURCEDISPLAYTYPE_SERVER):
		printf("server");
		break;
	case (RESOURCEDISPLAYTYPE_SHARE):
		printf("share");
		break;
	case (RESOURCEDISPLAYTYPE_FILE):
		printf("file");
		break;
	case (RESOURCEDISPLAYTYPE_GROUP):
		printf("group");
		break;
	case (RESOURCEDISPLAYTYPE_NETWORK):
		printf("network");
		break;
	default:
		printf("unknown %X", lpnrLocal->dwDisplayType);
		break;
	}	
	printf("\n\n");
}



int GetMacAddress() {
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	UINT i;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL) {
		printf("Error allocating memory needed to call GetAdaptersinfo\n");
		return 1;
	}

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(ulOutBufLen);
		if (pAdapterInfo == NULL) {
			printf("Error allocating memory needed to call GetAdaptersinfo\n");
			return 1;
		}
	}


	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;
		while (pAdapter) {
			printf("\tNetwork Adapter: \t%s\n", pAdapter->Description);
			printf("\tPhysical Address: \t");

			for (i = 0; i < pAdapter->AddressLength; i++) {
				if (i == (pAdapter->AddressLength - 1))
					printf("%.2X\n", (int)pAdapter->Address[i]);
				else
					printf("%.2X-", (int)pAdapter->Address[i]);
			}
			printf("\tAdapter Name: \t%s\n", pAdapter->AdapterName);
			pAdapter = pAdapter->Next;
			printf("\n");
		}
	}
	else {
		printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
	}
	if (pAdapterInfo)
		FREE(pAdapterInfo);
	return 0;
}


int main()
{
	printf("MAC-addresses: \n");
	GetMacAddress();
	printf("\nShowing all working groups, computers in the network and their resources: \n");
	enumerateResources();
	return 0;
}
