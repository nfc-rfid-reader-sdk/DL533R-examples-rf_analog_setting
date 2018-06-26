/*
 ============================================================================
 Name        : main.c
 Author      : Digital Logic
 Version     :
 Copyright   : www.d-logic.net
 Description : PR533 RF settings for typeA targets example
 ============================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <winscard.h>

SCARDHANDLE CardHandle;
SCARDCONTEXT ContextHnd;

unsigned int ResponseBufferLength = 255;
BYTE pResponseBuffer[255];
unsigned int ActiveProtocol = 0;

unsigned char setting_type_A_max_gain[] =
{ 0xFF, 0xE1, 0x04, 0x0A, 0x0B, 0x7A, 0xF4, 0x3F, 0x11, 0x4D, 0x85, 0x61, 0x6F, 0x26, 0x62, 0x87 };
unsigned char setting_type_A_default_gain[] =
{ 0xFF, 0xE1, 0x04, 0x0A, 0x0B, 0x5A, 0xF4, 0x3F, 0x11, 0x4D, 0x85, 0x61, 0x6F, 0x26, 0x62, 0x87 };

BOOL PR533_activate(void)
{
	unsigned int retVal = 0;
	DWORD ScAutoAlloc = SCARD_AUTOALLOCATE;
	LPSTR pmszReaders = NULL;
	char szPR533[] = "NXP PR533 0";

	unsigned int ProtocolType = 0, ActiveProtocol = 0;

	unsigned char get_fw[] =
	{ 0xFF, 0xE1, 0x00, 0x00, 0x04 };
	unsigned char stop_ACD[] =
	{ 0xFF, 0xE1, 0x04, 0x01, 0x01, 0x01, 0x00 };
	unsigned char start_ACD[] =
	{ 0xFF, 0xE1, 0x04, 0x01, 0x01, 0x01, 0x01 };
	//define manufacturer's ESCAPE string for PCSC reader
#define  IOCTL_MS_CCID_ESCAPE   0x003136B0
#define IOCTL_CSB6_PCSC_ESCAPE  0x00312000
#define IOCTL_CCID_ESCAPE SCARD_CTL_CODE(3500)

	/* Open the Context which communicate with SCARD resource manager  */
	retVal = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &ContextHnd);
	if (retVal != SCARD_S_SUCCESS)
	{
		printf("SCardEstablishContext error = %x\n", retVal);
		return FALSE;
	}
	else
	{
		retVal = SCardListReadersA(ContextHnd, NULL, (LPSTR) &pmszReaders, &ScAutoAlloc);
		if (SCARD_S_SUCCESS == retVal)
		{ /*  //0 - pmszReaders now contains the Reader name if existing. (The first one found is used) */
			//pReaders = pmszReaders;
			if (strcmp(szPR533, pmszReaders) == 0)
			{
				// Establish the connection with the reader (DIRECT)
				ProtocolType = SCARD_PROTOCOL_T1;
				retVal = SCardConnectA(ContextHnd, (LPCSTR) "NXP PR533 0",
				SCARD_SHARE_EXCLUSIVE, ProtocolType, &CardHandle, (LPDWORD) &ActiveProtocol);
				if (retVal != SCARD_S_SUCCESS)
				{
					//disable and enable again automatic tag discovery
					ProtocolType = SCARD_PROTOCOL_UNDEFINED;
					retVal = SCardConnectA(ContextHnd, (LPCSTR) "NXP PR533 0",
					SCARD_SHARE_DIRECT, ProtocolType, &CardHandle, (LPDWORD) &ActiveProtocol);

					SCardControl(CardHandle, IOCTL_CCID_ESCAPE, stop_ACD, sizeof(stop_ACD), pResponseBuffer, 255, (LPDWORD) &ResponseBufferLength);

					Sleep(20);

					SCardControl(CardHandle, IOCTL_CCID_ESCAPE, start_ACD, sizeof(start_ACD), pResponseBuffer, 255, (LPDWORD) &ResponseBufferLength);

					Sleep(50);

					SCardReleaseContext(ContextHnd);
					printf("SCardConnectA error = %x\n", retVal);
					return FALSE;
				}
				else
				{
					retVal =
							SCardControl(CardHandle, IOCTL_CCID_ESCAPE, get_fw, sizeof(get_fw), pResponseBuffer, 255, (LPDWORD) &ResponseBufferLength);
					if (retVal != SCARD_S_SUCCESS)
					{
						SCardReleaseContext(ContextHnd);
						printf("PR533 get firmware version error = %x\n", retVal);
						return FALSE;
					}
					else
					{
						if (pResponseBuffer[0] != 0x33)
						{
							SCardReleaseContext(ContextHnd);
							printf("PR533 get firmware version wrong = %x\n", pResponseBuffer[0]);
							return FALSE;
						}
						return TRUE;
					}
				}
			}
			else
			{
				SCardReleaseContext(ContextHnd);
				printf("PR533 not found \n");
				printf("Found %s\n", pmszReaders);
				return FALSE;
			}
		}
		else
		{
			SCardReleaseContext(ContextHnd);
			printf("SCardListReadersA error = %x\n", retVal);
			return FALSE;
		}
	}
}

void print_hex(unsigned char *pbtData, unsigned char szBytes)
{
	unsigned char szPos;

	for (szPos = 0; szPos < szBytes; szPos++)
	{
		printf("%02x  ", pbtData[szPos]);
	}
	printf("\n");
}

void main_menu(void)
{
	printf("\r\n\rMAIN MENU\n\r\n");
	printf("1 - SET THE MAXIMUM GAIN (RFCfg Register = 0x7A)\r\n");
	printf("2 - SET THE DEFAULT GAIN (RFCfg Register = 0x5A)\r\n");
	printf("3 - DETECT CARD\r\n");
	printf("4 - EXIT\r\n");
	printf("\r\nCHOOSE AN OPERATION (1 - 4):");
}

void rf_setting_typeA_max_gain(void)
{
	unsigned int retVal;

	retVal =
			SCardControl(CardHandle, IOCTL_CCID_ESCAPE, setting_type_A_max_gain, sizeof(setting_type_A_max_gain), pResponseBuffer, 255, (LPDWORD) &ResponseBufferLength);
	if (retVal != SCARD_S_SUCCESS)
	{
		if (retVal == SCARD_W_REMOVED_CARD)
		{
			Sleep(10);
			retVal = SCardReconnect(CardHandle, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T1, SCARD_LEAVE_CARD, (LPDWORD) &ActiveProtocol);
			retVal = SCardControl(CardHandle, IOCTL_CCID_ESCAPE, setting_type_A_max_gain, sizeof(setting_type_A_max_gain), pResponseBuffer, 255, (LPDWORD) &ResponseBufferLength);
			if(retVal == SCARD_S_SUCCESS)
				printf("RF settings OK\r\n");
			else
				printf("RF settings did not success\r\n");
		}
		else
		{
			SCardReleaseContext(ContextHnd);
			PR533_activate();
			printf("RF settings did not success\r\n");
		}
	}
	else
	{
		if (pResponseBuffer[0] != 0x90)
		{
			SCardReleaseContext(ContextHnd);
			printf("RF settings did not success\r\n");
			PR533_activate();
		}

		printf("RF settings OK\r\n");
	}
}

void rf_setting_typeA_default_gain(void)
{
	unsigned int retVal;

	retVal =
			SCardControl(CardHandle, IOCTL_CCID_ESCAPE, setting_type_A_default_gain, sizeof(setting_type_A_default_gain), pResponseBuffer, 255, (LPDWORD) &ResponseBufferLength);
	if (retVal != SCARD_S_SUCCESS)
	{
		if (retVal == SCARD_W_REMOVED_CARD)
		{
			Sleep(10);
			retVal = SCardReconnect(CardHandle, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T1, SCARD_LEAVE_CARD, (LPDWORD) &ActiveProtocol);
			retVal = SCardControl(CardHandle, IOCTL_CCID_ESCAPE, setting_type_A_default_gain, sizeof(setting_type_A_default_gain), pResponseBuffer, 255, (LPDWORD) &ResponseBufferLength);
			if(retVal == SCARD_S_SUCCESS)
				printf("RF settings OK\r\n");
			else
				printf("RF settings did not success\r\n");
		}
		else
		{
			SCardReleaseContext(ContextHnd);
			PR533_activate();
			printf("RF settings did not success\r\n");
		}
	}
	else
	{
		if (pResponseBuffer[0] != 0x90)
		{
			SCardReleaseContext(ContextHnd);
			printf("RF settings did not success\r\n");
			PR533_activate();
		}

		printf("RF settings OK\r\n");
	}
}

void card_detection(void)
{
	unsigned int retVal = 0;

	unsigned int ResponseBufferLength = 255;
	SCARD_IO_REQUEST IO_Request;

	BYTE pResponseBuffer[255];
	unsigned char GetUID[] =
	{ 0xFF,/* APDU Class */
	0xCA,/* INS as defined by PCSC */
	0x00,/* P1 */
	0x00,/* P2 */
	0x00 /* Le byte */
	};

	IO_Request.dwProtocol = SCARD_PROTOCOL_T1;
	IO_Request.cbPciLength = sizeof(SCARD_IO_REQUEST);

	retVal = SCardTransmit(CardHandle, &IO_Request, GetUID, sizeof(GetUID), 0, pResponseBuffer, (LPDWORD) &ResponseBufferLength);
	if (retVal == SCARD_S_SUCCESS)
	{
		printf("CARD DETECTED\r\n");
		printf("UID = ");
		print_hex(pResponseBuffer, ResponseBufferLength - 2);
	}
	else
	{
		if (retVal == SCARD_W_REMOVED_CARD)
		{
			Sleep(100);
			retVal =
					SCardReconnect(CardHandle, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_T1, SCARD_LEAVE_CARD, (LPDWORD) &ActiveProtocol);
			if (retVal == SCARD_S_SUCCESS)
			{
				retVal =
						SCardTransmit(CardHandle, &IO_Request, GetUID, sizeof(GetUID), 0, pResponseBuffer, (LPDWORD) &ResponseBufferLength);
				if (retVal == SCARD_S_SUCCESS)
				{
					printf("CARD DETECTED\r\n");
					printf("UID = ");
					print_hex(pResponseBuffer, ResponseBufferLength - 2);
				}
				else
				{
					printf("CARD DID NOT DETECT\r\n");
				}
			}
			else
			{
				printf("CARD DID NOT DETECT\r\n");
			}
		}
		else
		{
			SCardReleaseContext(ContextHnd);
			PR533_activate();
			printf("CARD DID NOT DETECT\r\n");
		}
	}
}

int main()
{
	char menu_choice;
	char menu_str[3];
	char *tp;

	if (PR533_activate())
		printf("DLPR533 reader detected\r\n");
	else
	{
		printf("DLR533 reader did not detect\r\n");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		main_menu();
		scanf("%s", menu_str);
		menu_choice = strtoul(menu_str, &tp, 10);

		switch (menu_choice)
		{
		case 1:
			rf_setting_typeA_max_gain();
			break;
		case 2:
			rf_setting_typeA_default_gain();
			break;
		case 3:
			card_detection();
			break;
		case 4:
			exit(EXIT_SUCCESS);
			break;
		}
	}
}
